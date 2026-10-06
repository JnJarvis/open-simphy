# Lightweight RFC process

Use TEMPLATE.md for shared API/schema changes, new dependency edges, technology
choices with broad impact, or materially changed scope. Small private changes that
preserve the contract need no RFC.

States: PROPOSED → ACCEPTED or REJECTED; accepted decisions get an adr/ entry.
The coordinator obtains affected owners' and integration owner's review, records
review identities and rationale, then schedules migrations. If no owner exists,
the designated integration reviewer fills that role. No response is not acceptance.
Block affected tasks until required migration prerequisites are merged.
