#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstring>
#include <locale>
#include <miniz.h>
#include <numbers>
#include <opensim/compat/ssim.hpp>
#include <pugixml.hpp>
#include <set>
#include <sstream>
namespace opensim::compat {
namespace {
core::Result<Project> reject(const std::string &reason) {
    return core::Result<Project>::failure(
        {core::Code::invalid_argument, core::Severity::error, reason, {}, "compat.ssim"});
}
struct Zip {
    mz_zip_archive value{};
    ~Zip() {
        if (value.m_pState)
            mz_zip_reader_end(&value);
    }
};
struct Stream {
    std::vector<std::uint8_t> *xml;
    std::uint64_t expected, written = 0;
};
size_t receive(void *opaque, mz_uint64 offset, const void *data, size_t length) {
    auto &s = *static_cast<Stream *>(opaque);
    if (offset != s.written || length > s.expected - s.written)
        return 0;
    if (s.xml) {
        const auto *begin = static_cast<const std::uint8_t *>(data);
        s.xml->insert(s.xml->end(), begin, begin + length);
    }
    s.written += length;
    return length;
}
bool safe_name(const std::string &name) {
    if (name.empty() || name.front() == '/' || name.find_first_of("\\:") != std::string::npos ||
        name.find('\0') != std::string::npos)
        return false;
    std::size_t start = 0;
    while (start < name.size()) {
        auto end = name.find('/', start);
        if (end == std::string::npos)
            end = name.size();
        const auto part = name.substr(start, end - start);
        if (part.empty() || part == "." || part == "..")
            return false;
        start = end + 1;
    }
    return true;
}
std::string type(pugi::xml_node node) {
    for (auto a : node.attributes()) {
        const std::string name = a.name();
        const auto colon = name.find(':');
        if (colon == std::string::npos || name.substr(colon + 1) != "type")
            continue;
        const auto declaration = "xmlns:" + name.substr(0, colon);
        for (auto parent = node; parent; parent = parent.parent())
            if (auto ns = parent.attribute(declaration.c_str())) {
                if (std::string(ns.value()) == "http://www.w3.org/2001/XMLSchema-instance")
                    return a.value();
                break;
            }
    }
    return "(unspecified)";
}
bool utf8(std::span<const std::uint8_t> bytes) {
    for (std::size_t i = 0; i < bytes.size();) {
        const auto first = bytes[i++];
        if (first < 128)
            continue;
        unsigned following = 0;
        std::uint32_t value = 0, minimum = 0;
        if (first >= 0xc2 && first <= 0xdf) {
            following = 1;
            value = first & 31;
            minimum = 128;
        } else if (first >= 0xe0 && first <= 0xef) {
            following = 2;
            value = first & 15;
            minimum = 2048;
        } else if (first >= 0xf0 && first <= 0xf4) {
            following = 3;
            value = first & 7;
            minimum = 65536;
        } else
            return false;
        if (following > bytes.size() - i)
            return false;
        while (following--) {
            auto next = bytes[i++];
            if ((next & 0xc0) != 0x80)
                return false;
            value = (value << 6) | (next & 63);
        }
        if (value < minimum || value > 0x10ffff || (value >= 0xd800 && value <= 0xdfff))
            return false;
    }
    return true;
}
bool scalar(const char *text, double &value) {
    std::istringstream input(text);
    input.imbue(std::locale::classic());
    if (!(input >> value) || !std::isfinite(value))
        return false;
    input >> std::ws;
    return input.eof();
}
bool vector(pugi::xml_node node, math::Vec2 &v) {
    return node && scalar(node.attribute("x").value(), v.x) &&
           scalar(node.attribute("y").value(), v.y);
}
void outline(Project &project, pugi::xml_node shape) {
    auto body = shape.parent();
    while (body && std::string(body.name()) != "Body" && std::string(body.name()) != "PlaneBody")
        body = body.parent();
    if (!body)
        return;
    const auto label = std::string(body.attribute("Name").value());
    const auto kind = type(shape);
    const auto missing = [&] {
        ++project.omitted_outlines;
        if (project.diagnostics.size() < 128)
            project.diagnostics.push_back("Geometry omitted: " + label + " / " + kind);
    };
    math::Vec2 translation{}, local{};
    double angle = 0, local_angle = 0;
    if (!vector(body.child("Transform").child("Translation"), translation) ||
        !scalar(body.child("Transform").child("Rotation").child_value(), angle)) {
        missing();
        return;
    }
    if (shape.child("LocalCenter") && !vector(shape.child("LocalCenter"), local)) {
        missing();
        return;
    }
    if (shape.child("LocalRotation") &&
        !scalar(shape.child("LocalRotation").child_value(), local_angle)) {
        missing();
        return;
    }
    if (project.outlines.size() >= 4096) {
        missing();
        return;
    }
    Outline result{label, {}, kind != "Segment"};
    if (kind == "Circle") {
        double radius = 0;
        if (!scalar(shape.child("Radius").child_value(), radius) || radius <= 0) {
            missing();
            return;
        }
        for (int i = 0; i < 64; ++i) {
            const double a = i * 2 * std::numbers::pi / 64;
            result.points.push_back({radius * std::cos(a), radius * std::sin(a)});
        }
    } else if (kind == "Rectangle") {
        double width = 0, height = 0;
        if (!scalar(shape.child("Width").child_value(), width) ||
            !scalar(shape.child("Height").child_value(), height) || width <= 0 || height <= 0) {
            missing();
            return;
        }
        result.points = {{-width / 2, -height / 2},
                         {width / 2, -height / 2},
                         {width / 2, height / 2},
                         {-width / 2, height / 2}};
    } else if (kind == "Polygon" || kind == "Triangle" || kind == "Segment") {
        for (auto vertex : shape.children("Vertex")) {
            math::Vec2 point{};
            if (!vector(vertex, point) || result.points.size() >= 4096) {
                missing();
                return;
            }
            result.points.push_back(point);
        }
        if (result.points.size() < 2) {
            missing();
            return;
        }
    } else {
        missing();
        return;
    }
    // Source file rotations are degrees. Shapes rotate about their local origin,
    // then translate to LocalCenter, then undergo the body transform.
    const double a = angle * std::numbers::pi / 180, b = local_angle * std::numbers::pi / 180;
    for (auto &v : result.points) {
        const math::Vec2 q{v.x * std::cos(b) - v.y * std::sin(b) + local.x,
                           v.x * std::sin(b) + v.y * std::cos(b) + local.y};
        v = {q.x * std::cos(a) - q.y * std::sin(a) + translation.x,
             q.x * std::sin(a) + q.y * std::cos(a) + translation.y};
        if (!math::finite(v) || std::abs(v.x) > 1e9 || std::abs(v.y) > 1e9) {
            missing();
            return;
        }
    }
    if (result.points.size() > 262144 - project.preview_points) {
        missing();
        return;
    }
    project.preview_points += result.points.size();
    project.outlines.push_back(std::move(result));
}
} // namespace
core::Result<Project> inspect_ssim(std::span<const std::uint8_t> bytes) {
    if (bytes.empty() || bytes.size() > max_ssim_bytes)
        return reject("Archive byte limit or empty input");
    Zip zip;
    if (!mz_zip_reader_init_mem(&zip.value, bytes.data(), bytes.size(), 0))
        return reject("Invalid ZIP archive");
    if (mz_zip_is_zip64(&zip.value))
        return reject("ZIP64 is not supported");
    const auto count = mz_zip_reader_get_num_files(&zip.value);
    if (count > 4096)
        return reject("Archive member limit");
    Project project;
    std::set<std::string> names;
    std::vector<std::uint8_t> xml;
    std::uint64_t expanded = 0;
    bool found = false;
    for (mz_uint i = 0; i < count; ++i) {
        mz_zip_archive_file_stat stat{};
        if (!mz_zip_reader_file_stat(&zip.value, i, &stat))
            return reject("Invalid member metadata");
        const auto length = mz_zip_reader_get_filename(&zip.value, i, nullptr, 0);
        if (length == 0 || length > 4096)
            return reject("Member name limit");
        std::string name(length, '\0');
        mz_zip_reader_get_filename(&zip.value, i, name.data(), length);
        name.pop_back();
        if (!safe_name(name) || !names.insert(name).second)
            return reject("Unsafe or duplicate member name");
        if ((stat.m_bit_flag & 1) || (stat.m_external_attr >> 16 & 0170000) == 0120000)
            return reject("Encrypted or linked member");
        if (stat.m_method != 0 && stat.m_method != 8)
            return reject("Unsupported compression");
        if (stat.m_uncomp_size > 128 * 1024 * 1024 - expanded ||
            stat.m_uncomp_size > 1000 * std::max<mz_uint64>(1, stat.m_comp_size))
            return reject("Archive expansion limit");
        expanded += stat.m_uncomp_size;
        if (stat.m_uncomp_size == 0 && stat.m_crc32 != 0)
            return reject("Empty member CRC mismatch");
        const bool is_xml = name == "simulation.xml";
        if (is_xml && stat.m_uncomp_size > 8 * 1024 * 1024)
            return reject("XML byte limit");
        if (is_xml)
            xml.reserve(static_cast<std::size_t>(stat.m_uncomp_size));
        Stream stream{is_xml ? &xml : nullptr, stat.m_uncomp_size};
        if (!mz_zip_reader_extract_to_callback(&zip.value, i, receive, &stream, 0) ||
            stream.written != stat.m_uncomp_size)
            return reject("Member data or CRC failure: " + name);
        project.members.push_back(std::move(name));
        found = found || is_xml;
    }
    if (!found || xml.empty())
        return reject("Missing or empty simulation.xml");
    if (std::find(xml.begin(), xml.end(), 0) != xml.end())
        return reject("Only UTF-8 XML is supported");
    if (!utf8(xml))
        return reject("Invalid UTF-8 XML");
    std::string text(xml.begin(), xml.end()), upper = text;
    std::transform(upper.begin(), upper.end(), upper.begin(),
                   [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
    if (upper.find("<!DOCTYPE") != std::string::npos || upper.find("<!ENTITY") != std::string::npos)
        return reject("DTD/entities forbidden");
    std::size_t starts = 0;
    for (std::size_t i = 0; i + 1 < text.size(); ++i)
        if (text[i] == '<' && text[i + 1] != '/' && text[i + 1] != '!' && text[i + 1] != '?' &&
            ++starts > 200000)
            return reject("XML element limit");
    pugi::xml_document document;
    const auto parsed =
        document.load_buffer(xml.data(), xml.size(), pugi::parse_default, pugi::encoding_utf8);
    if (!parsed)
        return reject("Malformed XML: " + std::string(parsed.description()));
    const auto root = document.document_element();
    if (std::string(root.name()) != "Simulation" || root.next_sibling())
        return reject("Expected one Simulation root");
    project.version = root.attribute("version").value();
    if (project.version != "4.0" && project.version != "4.1" && project.version != "4.2")
        return reject("Unsupported SSIM version: " + project.version);
    project.title = root.attribute("title").value();
    std::vector<std::pair<pugi::xml_node, unsigned>> pending{{root, 1}};
    std::size_t nodes = 0;
    while (!pending.empty()) {
        auto [node, depth] = pending.back();
        pending.pop_back();
        if (depth > 128 || ++nodes > 200000)
            return reject("XML depth or element limit");
        const std::string tag = node.name();
        ++project.elements[tag];
        if (tag == "Shape" && std::string(node.parent().name()) == "Fixture") {
            ++project.shapes[type(node)];
            outline(project, node);
        }
        if (tag == "Joint")
            ++project.joints[type(node)];
        if (tag == "Script" &&
            std::string(node.child_value()).find_first_not_of(" \t\r\n") != std::string::npos)
            ++project.scripts;
        for (auto child = node.last_child(); child; child = child.previous_sibling())
            if (child.type() == pugi::node_element)
                pending.emplace_back(child, depth + 1);
    }
    project.diagnostics.insert(
        project.diagnostics.begin(),
        "Source preview: physics, joints, scripts and assets are not executed.");
    project.diagnostics.push_back("Only directly defined fixture outlines are drawn; clones and "
                                  "other content remain in the source.");
    project.diagnostics.push_back("Omitted fixture outlines: " +
                                  std::to_string(project.omitted_outlines));
    project.source.assign(bytes.begin(), bytes.end());
    return core::Result<Project>::success(std::move(project));
}
} // namespace opensim::compat
