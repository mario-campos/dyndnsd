project('dyndnsd', 'C', version : '0.0.0', default_options : ['c_std=c99'])

config = configuration_data()
config.set('VERSION', '"@0@"'.format(meson.project_version()))
configure_file(output : 'config.h', configuration : config)

pathnames = configuration_data()
pathnames.set('ETCDIR', '"/etc"')
pathnames.set('_PATH_DYNDNSD_CONF', 'ETCDIR "/dyndnsd.conf"')
configure_file(output : 'pathnames.h', configuration : pathnames)

limits = configuration_data()
limits.set('LOG_MEM_LIMIT', '1024')
limits.set('URL_MEM_LIMIT', '1024')
limits.set('READ_MEM_LIMIT', '2048')
configure_file(output : 'limits.h', configuration : limits)

lex_src = custom_target('lex', input : 'src/lexer.l', output : 'lexer.c',
                        command : ['lex', '-o', 'lexer.c', '@INPUT@'])

bison_src = custom_target('bison', input : 'src/parser.y', output: ['parser.c', 'parser.h'],
                          command : ['bison', '-d', '-o', 'parser.c', '@INPUT@'])

sources = ['src/dyndnsd.c', 'src/ast.c', 'src/rtm.c', lex_src, bison_src]
includes = [include_directories('src'), include_directories('/usr/local/include')]

executable('dyndnsd', sources, include_directories : includes,
           dependencies : dependency('libcurl'),
           c_args : ['-Wall', '-Wextra', '-pedantic'])
