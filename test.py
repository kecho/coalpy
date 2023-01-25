import coalpy.gpu as g
g.get_settings().graphics_api = 'vulkan'
g.get_settings().spirv_debug_reflection = 'hello'
g.init()

s = g.Shader(name = "test_shader", main_function="csMain", file="shader_test.hlsl")
s.resolve()

def buildUi(imgui, implot):
    imgui.show_demo_window()
    implot.show_demo_window()

def doRender(renderArgs):
    buildUi(renderArgs.imgui, renderArgs.implot)
    cmdList = g.CommandList() 
    cmdList.dispatch(
        shader = s,
        constants = [float(1.0), float(0.0), float(1.0), float(1.0)],
        outputs = renderArgs.window.display_texture,
        x = int(renderArgs.width/8),
        y = int(renderArgs.height/8),
        z = 1)
    g.schedule(cmdList)

w = g.Window(height=512, width=512, on_render = doRender);
g.run();
