import os
files = ['folder.svg', 'file.svg', 'photo.svg', 'video.svg', 'audio.svg']
with open('include/assets.h', 'w') as out:
    out.write('#pragma once\n\n')
    for f in files:
        path = os.path.join('assets', f)
        if not os.path.exists(path): continue
        with open(path, 'r') as infile:
            content = infile.read()
        content = content.replace('\\', '\\\\').replace('"', '\\"').replace('\n', '\\n')
        var_name = f.replace('\.svg', '_svg')
        out.write(f'const char* asset_{var_name} = "{content}";\n\n')
