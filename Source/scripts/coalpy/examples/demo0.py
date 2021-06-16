import coalpy.gpu as gpu

s1 = gpu.inlineShader("testShader", """
    #include "coalpy/examples/testInclude.hlsl"

    [numthreads(1,1,1)]
    void main()
    {
        testFn();
    }
""")

def onRender(renderArgs):
    s1.resolve()
    return 0   

def main():
    w = gpu.Window(1280, 720, onRender)
    gpu.run()

if __name__ == "__main__":
    main()
