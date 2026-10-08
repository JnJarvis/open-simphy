"""Independently authored native smoke fixture; no SimPHY XML/assets copied.
Usage: python tests/compatibility/ssim/INT-009-native.py build/elastic-smoke.ssim
Run opensim_demo --smoke-elastic against the resulting archive.
"""
import sys
import zipfile
from pathlib import Path

def body(ident, name, position, shape, static=False, mass=1):
    return f'<Body Id="{ident}" Name="{name}"><Transform><Translation x="{position[0]}" y="{position[1]}"/><Rotation>0</Rotation></Transform><Mass><LocalCenter x="0" y="0"/><Type>{"INFINITE" if static else "NORMAL"}</Type><Mass>{mass}</Mass><Inertia>1</Inertia></Mass><Fixtures><Fixture>{shape}<Friction>.3</Friction><Restitution>0</Restitution></Fixture></Fixtures><Velocity x="0" y="0"/><AngularVelocity>0</AngularVelocity><FillColor r=".7" g=".5" b=".2" a="1"/></Body>'

circle = '<Shape xsi:type="Circle"><LocalCenter x="0" y="0"/><Radius>.2</Radius></Shape>'
outline = [(0, 0), (3, 0), (3, 1), (1, 1), (1, 3), (0, 3)]
lshape = '<Shape xsi:type="Polygon"><LocalCenter x="0" y="0"/>' + ''.join(f'<Vertex x="{x}" y="{y}"/>' for x, y in outline) + '</Shape>'
bodies = ['<Body Id="g"><Mass><Type>INFINITE</Type></Mass><Fixtures/></Body>',
          body('s', 'spring', (-3, -1), circle, mass=2),
          body('r', 'rope', (0, -1), circle),
          body('p', 'notch', (2, -2), lshape, True),
          body('b', 'notch ball', (4, 0), circle),
          body('w1', 'weld A', (-3, 3), circle),
          body('w2', 'weld B', (-2, 3), circle)]
joints = '<Joint xsi:type="SpringJoint"><BodyId1>s</BodyId1><BodyId2>g</BodyId2><Anchor1 x="-3" y="-1"/><Anchor2 x="-3" y="1"/><distance>2</distance><SpringConstant>30</SpringConstant><DampingRatio>1</DampingRatio></Joint>'
joints += '<Joint xsi:type="RopeJoint"><BodyId1>r</BodyId1><BodyId2>g</BodyId2><Anchor1 x="0" y="-1"/><Anchor2 x="0" y="1"/><UpperLimit>3</UpperLimit><UpperLimitEnabled>true</UpperLimitEnabled></Joint>'
joints += '<Joint xsi:type="WeldJoint"><BodyId1>w1</BodyId1><BodyId2>w2</BodyId2><Anchor x="-2.5" y="3"/><ReferenceAngle>0</ReferenceAngle></Joint>'
script = "function reset(){for(const name of ['spring','rope','notch ball','weld A','weld B'])World.getBody(name).reset();}"
gui = '<desktop><button text="Reset" action="reset()" rectbounds="20,20,120,32"/><textarea text="Synthetic elastic, rope, weld and concave-notch smoke" rectbounds="20,70,300,50"/></desktop>'
xml = '<Simulation version="4.1" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"><World><Gravity x="0" y="-10"/><Bodies>' + ''.join(bodies) + '</Bodies><Joints>' + joints + '</Joints><ScriptManager><Script><![CDATA[' + script + ']]></Script></ScriptManager><GuiManager><GuiXML><![CDATA[' + gui + ']]></GuiXML></GuiManager></World></Simulation>'
output = Path(sys.argv[1])
output.parent.mkdir(parents=True, exist_ok=True)
with zipfile.ZipFile(output, 'w', zipfile.ZIP_DEFLATED) as archive:
    archive.writestr('simulation.xml', xml)
