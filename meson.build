project('Lkey', 'c')
# gnome = import('gnome')

# asresources = gnome.compile_resources('interface.ui', 'lkey.gresource.xml') 

gtk4_dep = dependency('gtk4')
jack_dep = dependency('jack')
deps = [gtk4_dep, jack_dep]
src = ['lkey.c', 'interface.c', 'resources.c']
executable('lkey', src, dependencies : deps)
