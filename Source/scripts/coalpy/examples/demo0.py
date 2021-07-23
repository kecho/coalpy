import sys
import math
import coalpy.gpu as gpu
import array

print ("<<coalpy demo0>>")
print ("graphics devices:")
[print("{}: {}".format(idx, nm)) for (idx, nm) in gpu.get_adapters()]

gpu.set_current_adapter(1)
info = gpu.get_current_adapter_info()
print("Current device: {}".format(info[1]))

gpu.add_shader_path(gpu) #this adds the main module in the paths.

s1 = gpu.Shader(file="examples/testShader.hlsl", name="testShader")

def main():

    def on_render(render_args : gpu.RenderArgs):

        cmdList = gpu.CommandList()
        xv = int(math.ceil((render_args.width)/8));
        yv = int(math.ceil((render_args.height)/4));
        dims = (1.0/render_args.width, 1.0/render_args.height)
        cmdList.dispatch(
            x = xv, y = yv, z = 1,
            constants = [float(render_args.render_time), dims[0], dims[1], 0],
            shader = s1,
            output_tables = output_table
        )
        gpu.schedule([cmdList])

        return 0   

    w = gpu.Window("coalpy demo 0", 1920, 1080, on_render)
    output_table = gpu.OutResourceTable("SwapTable", [w.display_texture])

    gpu.run()

if __name__ == "__main__":
    main()
