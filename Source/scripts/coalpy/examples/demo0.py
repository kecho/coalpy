import sys
import coalpy.gpu as gpu

print ("<<coalpy demo0>>")
print ("graphics devices:")
[print("{}: {}".format(idx, nm)) for (idx, nm) in gpu.get_adapters()]

info = gpu.get_current_adapter_info()
print("Current device: {}".format(info[1]))


s1 = gpu.inline_shader("testShader", """
    #include "coalpy/examples/testInclude.hlsl"

    [numthreads(1,1,1)]
    void main()
    {
        testFn();
    }
""")

s2 = gpu.Shader(name = "testShader2", main_function =  "csMain", file = "coalpy/examples/testShader.hlsl");

def on_render(render_args : gpu.RenderArgs):
    s1.resolve();
    s2.resolve();
    return 0   

def main():
    w = gpu.Window(1280, 720, on_render)
    gpu.run()

if __name__ == "__main__":
    main()
