# import the library
import coalpy.gpu

# define a simple rendering function (do nothing)
def render_fn(render_args : coalpy.gpu.RenderArgs):
    return

# create a window
w = coalpy.gpu.Window(title = "Tutorial", width = 400, height = 400, on_render = render_fn)

# do the window loop. This blocks, and unless w is set to None or the user hits the close button, it will continue
coalpy.gpu.run()

