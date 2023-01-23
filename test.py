import coalpy.gpu as g
g.get_settings().graphics_api = 'vulkan'
g.init()

s = g.Shader(name = "test_shader", main_function="csMain", file="shader_test.hlsl")
s.resolve()

def buildUi(imgui):
    w = True
    w = imgui.begin("Test window", w)
    imgui.end()
def doRender(renderArgs):
    buildUi(renderArgs.imgui)
    cmdList = g.CommandList() 
    cmdList.dispatch(
        shader = s,
        outputs = renderArgs.window.display_texture,
        x = int(renderArgs.width/8),
        y = int(renderArgs.height/8),
        z = 1)
    g.schedule(cmdList)

w = g.Window(height=512, width=512, on_render = doRender);
g.run();
