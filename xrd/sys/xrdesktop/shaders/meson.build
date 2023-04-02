shaders = ['pointer.vert', 'pointer.frag',
           'window.vert', 'window.frag',
           'device_model.vert', 'device_model.frag']

glslc = find_program('glslc', required : false)
if glslc.found()
  # Prefer shaderc
  cmd = [glslc]
else
  # Use glslang as fallback
  glslang = find_program('glslangValidator')
  if glslang.found()
    cmd = [glslang, '-V']
  endif
endif

foreach s : shaders
  r = run_command(cmd + ['-o', meson.current_build_dir() / (s + '.spv'), s])
  if r.returncode() != 0
    message('Could not compile shaders:')
    message(r.stderr().strip())
    message(r.stdout().strip())
  endif
endforeach

shader_resources = gnome.compile_resources(
  'shader_resources', 'shaders.gresource.xml',
  source_dir : meson.current_build_dir())
