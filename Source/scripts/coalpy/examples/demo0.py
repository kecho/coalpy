import coalpy.gpu as gpu

s1 = gpu.inlineShader("testShader", """
    #include "coalpy/examples/testInclude.hlsl"

    [numthreads(1,1,1)]
    void main()
    {
        testFn();
    }
""")

s2 = gpu.Shader(name = "testShader2", mainFunction =  "csMain", file = "coalpy/examples/testShader.hlsl");

def onRender(renderArgs : gpu.RenderArgs):
    s1.resolve();
    s2.resolve();
    return 0   

def main():
    w = gpu.Window(1280, 720, onRender)
    gpu.run()

if __name__ == "__main__":
    main()
