import coalpy.gpu as gpu
import coalpy.utils.profiler as profiler
import math

enable_profiler = True

print ("<<coalpy seascape>>")
try:
    import numpy
except ImportError:
    enable_profiler = False
    print("WARNING!! numpy is not installed, the profiler won't work. To use the profiler install numpy on your python packages.")
print ("graphics devices:")
[print("{}: {}".format(idx, nm)) for (idx, nm) in gpu.get_adapters()]

info = gpu.get_current_adapter_info()
print("Current device: {}".format(info[1]))

seascapeShader = gpu.Shader(file="examples/seascape.hlsl", name="seascape")
output_table = None

speed = 0.1
specular = 1.0
diffuse = 1.0
refraction = 1.0

profiler_obj = None
if enable_profiler:
    profiler_obj = profiler.Profiler()

def buildUi(imgui, implot, profiler_obj):
    global speed, specular, diffuse, refraction

    imgui.begin("Settings")
    speed = imgui.slider_float("speed", speed, 0.0, 1.0)
    specular = imgui.slider_float("specular", specular, 0.0, 1.0)
    diffuse = imgui.slider_float("diffuse", diffuse, 0.0, 1.0)
    refraction = imgui.slider_float("refraction", refraction, 0.0, 1.0)
    imgui.end()

    if profiler_obj != None:
        profiler_obj.build_ui(imgui, implot)

def on_render(render_args : gpu.RenderArgs):
    global profiler_obj
    global output_table
    global speed, specular, diffuse, refraction

    buildUi(render_args.imgui, render_args.implot, profiler_obj)

    if profiler_obj != None:
        profiler_obj.begin_capture()

    cmdList = gpu.CommandList()
    xv = int(math.ceil((render_args.width)/8));
    yv = int(math.ceil((render_args.height)/4));

    cmdList.begin_marker("seascape_dispatch")
    cmdList.dispatch(
        x = xv, y =yv, z = 1,
        constants = [
            float(render_args.width), float(render_args.height), float(0.04 * speed * render_args.render_time), 0.0,
            specular, diffuse, refraction, 0.0
        ],
        shader = seascapeShader,
        outputs = render_args.window.display_texture 
    )
    cmdList.end_marker()

    gpu.schedule(cmdList)

    if profiler_obj != None:
        profiler_obj.end_capture()

window = gpu.Window("seascape example", int(1090/2), int(1080/2), on_render)
output_table = gpu.OutResourceTable("output table", [window.display_texture])

if __name__ == "__main__":
    gpu.run()
