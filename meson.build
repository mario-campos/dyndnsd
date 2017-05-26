project('dyndnsd', 'c')

cdata = configuration_data()
cdata.set('PACKAGE', '"dyndnsd"')
cdata.set('PACKAGE_ETC_FILE', '"/etc/dyndnsd.conf"')
cdata.set('VERSION', '"0.0.0"')
configure_file(output : 'config.h', configuration : cdata)

lex_src = custom_target('lex', input : 'src/lexer.l', output : 'lexer.c',
                        command : ['lex', '-o', 'lexer.c', '@INPUT@'])

yacc_src = custom_target('yacc', input : 'src/parser.y', output: ['parser.c', 'parser.h'],
                         command : ['yacc', '-d', '-o', 'parser.c', '@INPUT@'])

executable('dyndnsd', 'src/dyndnsd.c', lex_src, yacc_src,
           include_directories : include_directories('src'))
