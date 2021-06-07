import coalpy.gpu as gpu

def onRender():
    return 0   

def main():
    w = gpu.Window(1280, 720, onRender)
    gpu.run()

if __name__ == "__main__":
    main()
