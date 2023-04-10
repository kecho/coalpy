import coalpy.gpu as gpu
import math

print ("<<coalpy demo0>>")
print ("graphics devices:")
[print("{}: {}".format(idx, nm)) for (idx, nm) in gpu.get_adapters()]

settings = gpu.get_settings()
settings.adapter_index = 0
settings.shader_model = "sm6_1"
settings.dump_shader_pdbs = True
settings.graphics_api = "vulkan"
gpu.init()

info = gpu.get_current_adapter_info()
print("Current device: {}".format(info[1]))

s1 = gpu.Shader(file="examples/testShader.hlsl", name="testShader") 

f = 0.5;
f2 = 0.5;
t = "hello"
b = False
h1 = False
w = True

sa = False
sb = False
sc = False

test_image = gpu.Texture(file="examples/data/test.png")
input_table = gpu.InResourceTable("testjpg_table", [test_image])

def buildUi(imgui):
    global f, f2, t, b, w
    global sa, sb, sc

    imgui.begin_main_menu_bar()
    if imgui.begin_menu("Tools"):
        if imgui.menu_item("Enable Window"):
            w = True
        imgui.end_menu()

    imgui.end_main_menu_bar()

    if w:
        w = imgui.begin("Menu Window", w)
        imgui.push_id("Menu Window")
        if imgui.collapsing_header("testHeader"):
            imgui.text("testing header contents")
            imgui.button("some button")
            imgui.button("some button2")
            f = imgui.slider_float("test slider", f, -1.0, 1.0);
            f2 = imgui.input_float("test float2", f2);
            t = imgui.input_text("input text box", t);
            imgui.text("some standard text");
            b = imgui.checkbox("checkbox test", b)

            if imgui.begin_combo("testCombo", "prevValue"):
                sa = imgui.selectable("sa", sa)
                sb = imgui.selectable("sb", sb)
                sc = imgui.selectable("sc", sc)
                imgui.end_combo()
        imgui.pop_id()

        imgui.image(test_image, (250,250))


        imgui.end();

def main():

    def on_render(render_args : gpu.RenderArgs):
        cmdList = gpu.CommandList()

        cmdList.begin_marker("beginDemo0")
        cmdList.begin_marker("beginDemoInternal")

        xv = int(math.ceil((render_args.width)/8));
        yv = int(math.ceil((render_args.height)/4));
        dims = (1.0/render_args.width, 1.0/render_args.height)
        cmdList.dispatch(
            x = xv, y = yv, z = 1,
            constants = [float(render_args.render_time), dims[0], dims[1], 0],
            shader = s1,
            inputs = test_image,
            outputs = render_args.window.display_texture
        )
        cmdList.end_marker()
        cmdList.end_marker()
        gpu.schedule([cmdList])

        buildUi(render_args.imgui)
        return 0   
    
    w = gpu.Window("coalpy demo 0", 1920, 1080, on_render)
    output_table = gpu.OutResourceTable("SwapTable", [w.display_texture])
    gpu.run()
if __name__ == "__main__":
    main()
