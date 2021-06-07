import coalpy.gpu as gpu

def onRender():
    return 0   

def main():
    w = gpu.Window(1280, 720, onRender)
    s1 = gpu.Shader()
    gpu.run()

if __name__ == "__main__":
    main()
