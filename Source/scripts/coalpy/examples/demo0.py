import sys
import coalpy.gpu as gpu
import array

print ("<<coalpy demo0>>")
print ("graphics devices:")
[print("{}: {}".format(idx, nm)) for (idx, nm) in gpu.get_adapters()]

gpu.set_current_adapter(1)
info = gpu.get_current_adapter_info()
print("Current device: {}".format(info[1]))


s1 = gpu.Shader(file="coalpy/examples/testShader.hlsl", name="testShader")

def main():

    def on_render(render_args : gpu.RenderArgs):

        cmdList = gpu.CommandList()
        xv = int((480 + 7)/8);
        yv = int((480 + 3)/4);
        cmdList.dispatch(
            x = xv, y = yv, z = 1,
            constants = [1,2,3,4, 0.71, 0.8, 0.1, 0.1],
            shader = s1,
            output_tables = output_table
        )
        gpu.schedule([cmdList])

        return 0   

    w = gpu.Window("coalpy demo 0", 480, 480, on_render)
    output_table = gpu.OutResourceTable("SwapTable", [w.display_texture])

    gpu.run()

if __name__ == "__main__":
    main()
