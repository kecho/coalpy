# import the library
import coalpy.gpu
import math

g_shader = coalpy.gpu.Shader(file="window_fx_shader_v1.hlsl", name="WindowFx", main_function="csMainWindow")


g_color = [0.1, 0.0, 0.93, 1.0]
def draw_imgui(imgui : coalpy.gpu.ImguiBuilder):
    # this is so we can write to the global g_color
    global g_color

    #begin panel
    imgui.begin("Options")

    # color picker
    col = imgui.color_edit4("effect color 0", g_color)

    # only write color if it was picked
    if col is not None:
        g_color = list(col)

    #end panel
    imgui.end()

# define a simple rendering function (do nothing)
def render_fn(render_args : coalpy.gpu.RenderArgs):
    global g_color

    # draw user interface
    draw_imgui(render_args.imgui)

    # create a new command list object
    cmd = coalpy.gpu.CommandList()

    # add a dispatch command
    cmd.dispatch(
        constants = g_color,
        shader = g_shader,
        outputs = render_args.window.display_texture,
        x = math.ceil(render_args.width/8),
        y = math.ceil(render_args.height/8),
        z = 1)

    # submit command list to the GPU for execution
    coalpy.gpu.schedule(cmd)
    return

# create a window
w = coalpy.gpu.Window(title = "Tutorial", width = 400, height = 400, on_render = render_fn)

# do the window loop. This blocks, and unless w is set to None or the user hits the close button, it will continue
coalpy.gpu.run()
