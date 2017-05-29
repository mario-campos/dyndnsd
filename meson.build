project('dyndnsd', 'C', version : '0.0.0', default_options : ['c_std=c99'])

cdata = configuration_data()
cdata.set('EXENAME', '"dyndnsd"')
cdata.set('ETCFILE', '"/etc/dyndnsd.conf"')
cdata.set('VERSION', '"0.0.0"')
configure_file(output : 'config.h', configuration : cdata)

lex_src = custom_target('lex', input : 'src/lexer.l', output : 'lexer.c',
                        command : ['lex', '-o', 'lexer.c', '@INPUT@'])

bison_src = custom_target('bison', input : 'src/parser.y', output: ['parser.c', 'parser.h'],
                         command : ['bison', '-d', '-o', 'parser.c', '@INPUT@'])

sources = ['src/dyndnsd.c', 'src/ast.c', lex_src, bison_src]

executable('dyndnsd', sources, include_directories : include_directories('src'))
