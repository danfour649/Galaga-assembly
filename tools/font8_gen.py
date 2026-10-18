# Original 8x8 bold arcade-style font (7x7 glyph in an 8x8 cell).
G = {}
def g(c, *rows):
    assert len(rows) == 7, c
    for r in rows: assert len(r) == 7, (c, r)
    G[c] = rows
g('0',".XXXXX.","XX...XX","XX..XXX","XX.X.XX","XXX..XX","XX...XX",".XXXXX.")
g('1',"...XX..","..XXX..","...XX..","...XX..","...XX..","...XX..",".XXXXXX")
g('2',".XXXXX.","XX...XX",".....XX","...XXX.",".XXX...","XX.....","XXXXXXX")
g('3',"XXXXXXX","....XX.","...XX..","....XX.",".....XX","XX...XX",".XXXXX.")
g('4',"...XXX.","..XXXX.",".XX.XX.","XX..XX.","XXXXXXX","....XX.","....XX.")
g('5',"XXXXXX.","XX.....","XXXXXX.",".....XX",".....XX","XX...XX",".XXXXX.")
g('6',"..XXXX.",".XX....","XX.....","XXXXXX.","XX...XX","XX...XX",".XXXXX.")
g('7',"XXXXXXX","XX...XX","....XX.","...XX..","..XX...","..XX...","..XX...")
g('8',".XXXX..","XX...X.","XXX..X.",".XXXX..","X..XXXX","X....XX",".XXXXX.")
g('9',".XXXXX.","XX...XX","XX...XX",".XXXXXX",".....XX","....XX.",".XXXX..")
g('A',"..XXX..",".XX.XX.","XX...XX","XX...XX","XXXXXXX","XX...XX","XX...XX")
g('B',"XXXXXX.","XX...XX","XX...XX","XXXXXX.","XX...XX","XX...XX","XXXXXX.")
g('C',"..XXXX.",".XX..XX","XX.....","XX.....","XX.....",".XX..XX","..XXXX.")
g('D',"XXXXX..","XX..XX.","XX...XX","XX...XX","XX...XX","XX..XX.","XXXXX..")
g('E',"XXXXXXX","XX.....","XX.....","XXXXXX.","XX.....","XX.....","XXXXXXX")
g('F',"XXXXXXX","XX.....","XX.....","XXXXXX.","XX.....","XX.....","XX.....")
g('G',"..XXXXX",".XX....","XX.....","XX..XXX","XX...XX",".XX..XX","..XXXXX")
g('H',"XX...XX","XX...XX","XX...XX","XXXXXXX","XX...XX","XX...XX","XX...XX")
g('I',".XXXXXX","...XX..","...XX..","...XX..","...XX..","...XX..",".XXXXXX")
g('J',"....XXX",".....XX",".....XX",".....XX",".....XX","XX...XX",".XXXXX.")
g('K',"XX...XX","XX..XX.","XX.XX..","XXXX...","XXXXX..","XX.XXX.","XX..XXX")
g('L',".XX....",".XX....",".XX....",".XX....",".XX....",".XX....",".XXXXXX")
g('M',"XX...XX","XXX.XXX","XXXXXXX","XXXXXXX","XX.X.XX","XX...XX","XX...XX")
g('N',"XX...XX","XXX..XX","XXXX.XX","XX.XXXX","XX..XXX","XX...XX","XX...XX")
g('O',".XXXXX.","XX...XX","XX...XX","XX...XX","XX...XX","XX...XX",".XXXXX.")
g('P',"XXXXXX.","XX...XX","XX...XX","XX...XX","XXXXXX.","XX.....","XX.....")
g('Q',".XXXXX.","XX...XX","XX...XX","XX...XX","XX.XXXX","XX..XX.",".XXXX.X")
g('R',"XXXXXX.","XX...XX","XX...XX","XX..XXX","XXXXX..","XX.XXX.","XX..XXX")
g('S',".XXXX..","XX..XX.","XX.....",".XXXXX.",".....XX","XX...XX",".XXXXX.")
g('T',"XXXXXX.","..XX...","..XX...","..XX...","..XX...","..XX...","..XX...")
g('U',"XX...XX","XX...XX","XX...XX","XX...XX","XX...XX","XX...XX",".XXXXX.")
g('V',"XX...XX","XX...XX","XX...XX","XXX.XXX",".XXXXX.","..XXX..","...X...")
g('W',"XX...XX","XX...XX","XX.X.XX","XXXXXXX","XXXXXXX","XXX.XXX","XX...XX")
g('X',"XX...XX","XXX.XXX",".XXXXX.","..XXX..",".XXXXX.","XXX.XXX","XX...XX")
g('Y',"XX..XX.","XX..XX.","XX..XX.",".XXXX..","..XX...","..XX...","..XX...")
g('Z',"XXXXXXX","....XXX","...XXX.","..XXX..",".XXX...","XXX....","XXXXXXX")
g('-',".......",".......",".......",".XXXXX.",".......",".......",".......")
g('.',".......",".......",".......",".......",".......","..XX...","..XX...")
g('!',"..XX...","..XX...","..XX...","..XX...","..XX...",".......","..XX...")
g(':',".......","..XX...","..XX...",".......","..XX...","..XX...",".......")
g('/',".....XX","....XX.","...XX..","..XX...",".XX....","XX.....","X......")
g('>',".XX....","..XX...","...XX..","....XX.","...XX..","..XX...",".XX....")
g('+',".......","..XX...","..XX...","XXXXXX.","..XX...","..XX...",".......")
g('%',"XX...X.","XX..XX.","...XX..","..XX...",".XX....","XX..XX.","X...XX.")
g('=',".......",".......","XXXXXX.",".......","XXXXXX.",".......",".......")
g('@',"..XXX..",".X...X.","X..XX.X","X.X...X","X..XX.X",".X...X.","..XXX..")  # copyright sign
chars = ''.join(G)
out = ["/* Generated: original 8x8 bold font (7x7 glyph in an 8x8 cell, bit 7 =",
       "   leftmost column). '@' draws the copyright sign. No ROM data. */",
       f'static const char FONT8_CHARS[] = "{chars}";',
       f"static const Uint8 FONT8[{len(chars)}][8] = {{"]
for c in chars:
    rows = [int(r.replace('X','1').replace('.','0') + '0', 2) for r in G[c]] + [0]
    out.append("    { " + ", ".join(f"0x{v:02x}" for v in rows) + f" }}, /* {c} */")
out.append("};")
open(__import__('os').path.join(__import__('os').path.dirname(__file__), '..', 'src', 'font8.h'),'w').write("\n".join(out) + "\n")
print(len(chars))
