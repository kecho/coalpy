import coalpy.gpu as g
import numpy as nm
import math as m

class Profiler:
    """
    Profiler class. Instantiate to utilize helper function for profiling. 
    Usage example:

        profiler = coalpy.Profiler()
        command_list = coalpy.gpu.CommandList()
        profiler.begin_capture() # starts the capture

        command_list.begin_marker("FirstMarker")
            command_list.begin_marker("SecondMarker")
            # .... some dispatches here ... 
            command_list.end_marker()
        command_list.end_marker()

        coalpy.gpu.submit(command_list)
        profiler.end_capture() # ends capture for all these submits

        # then later we can draw it via build_ui
        
        profiler.build_ui(render_args.imgui, render_args.implot)

    """
    def __init__(self):
        self.m_active = True
        self.m_gpu_queue = []
        self.m_marker_data = []
        self.m_plot_capacity = 200
        self.m_curr_tick = 0
        self.m_gpu_plot_data = nm.zeros((self.m_plot_capacity, 2), dtype='f')

    @property
    def active(self):
        return self.m_active

    @active.setter
    def active(self, value):
        self.m_active = value

    def build_ui(self, imgui : g.ImguiBuilder, implot : g.ImplotBuilder):
        """
        Builds a UI for this profiler user coalpy's imgui and implot objects.

        Parameters:
            imgui : the imgui builder object. Found in the render_args of a on_render callback.
            imgui : the implot builder object. Found in the render_args of a on_render callback.
        """
        self.m_active = imgui.begin("Profiler", self.m_active)
        if self.m_active and imgui.begin_tab_bar("profiler-tab"):
            if imgui.begin_tab_item("Timeline"):
                self._build_timeline_ui(imgui, implot)
                imgui.end_tab_item()
            if imgui.begin_tab_item("Hierarchy"):
                self._build_hierarchy_ui(imgui)
                imgui.end_tab_item()
            if imgui.begin_tab_item("Raw Counters"):
                self._build_raw_counter_ui(imgui)
                imgui.end_tab_item()
            imgui.end_tab_bar()
        imgui.end()

    def _build_raw_counter_ui(self, imgui : g.ImguiBuilder):
        titles = ["ID", "ParentID", "Name", "Time", "BeginTimestamp", "EndTimestamp"]
        imgui.text(f"{titles[0] : <4} {titles[1] : <8} {titles[2] : <32} {titles[3] : ^10} {titles[4] : ^18} {titles[5] : ^18} ")
        for id in range(0, len(self.m_marker_data)):
            (name, end_timestamp, begin_timestamp, parent_id) = self.m_marker_data[id]
            time = end_timestamp - begin_timestamp
            time_str = "%.4f ms" % (time * 1000)
            imgui.text(f"{id: <4} {parent_id : <8} {name : <32} {time_str : ^10} {begin_timestamp : ^18} {end_timestamp : ^18} ")

    def _build_hierarchy_ui(self, imgui : g.ImguiBuilder):
        if len(self.m_marker_data) == 0:
            return

        hierarchy = [(id, []) for id in range(0, len(self.m_marker_data))]
        node_stack = []
        for id in range(0, len(self.m_marker_data)):
            (_, _, _, parent_id) = self.m_marker_data[id]
            if parent_id != -1:
                hierarchy[parent_id][1].append(id)
            else:
                node_stack.append((id, False))

        node_stack.reverse()
        for (_, l) in hierarchy:
            l.reverse()

        while len(node_stack) > 0:
            (id, was_visited) = node_stack.pop()
            if was_visited:
                imgui.tree_pop()
            else:
                (name, timestamp_end, timestamp_begin, _) = self.m_marker_data[id]
                children = hierarchy[id][1]
                flags = (g.ImGuiTreeNodeFlags.Leaf|g.ImGuiTreeNodeFlags.Bullet) if len(children) == 0 else 0
                timestamp_str = "%.4f ms" % ((timestamp_end - timestamp_begin) * 1000)
                if imgui.tree_node_with_id(id, f"{name : <32}{timestamp_str}", flags):
                    node_stack.append((id, True)) #set was_visited to True
                    node_stack.extend([(child_id, False) for child_id in children])

    def _build_timeline_ui(self, imgui : g.ImguiBuilder, implot : g.ImplotBuilder):
        if implot.begin_plot("Timeline"):
            implot.setup_axes("Tick", "Time (ms)", 0, g.ImPlotAxisFlags.AutoFit)
            implot.setup_axis_limits(g.ImAxis.X1, self.m_curr_tick - self.m_plot_capacity, self.m_curr_tick, g.ImPlotCond.Always)
            implot.plot_shaded("gpu time", self.m_gpu_plot_data, self.m_plot_capacity, -float('inf'),(self.m_curr_tick % self.m_plot_capacity))
            implot.end_plot()

    def begin_capture(self):
        """
        Begins the capture of all the following scheduled command lists. Note: you can write command lists outside this method.
        Whats important is that all the coalpy.gpu.schedule calls are wrapped by a begin_capture and end_capture
        """
        if not self.active:
            return

        g.begin_collect_markers()

    def end_capture(self):        
        """
        Ends the capture of all the previously scheduled command lists. See begin_capture.
        """
        if not self.active:
            return

        marker_gpu_data = g.end_collect_markers()
        request = g.ResourceDownloadRequest(marker_gpu_data.timestamp_buffer)
        self.m_gpu_queue.append((marker_gpu_data, request))

        if self.m_gpu_queue[0][1].is_ready():
            #extract markers
            (data, req) = self.m_gpu_queue.pop(0)
            gpu_timestamps = nm.frombuffer(req.data_as_bytearray(), dtype=nm.uint64)
            self.m_marker_data = [ (name, gpu_timestamps[ei]/data.timestamp_frequency, gpu_timestamps[bi]/data.timestamp_frequency, pid) for (name, pid, bi, ei) in data.markers]

            #process history
            root_tstamps = [(b, e) for (_, e, b, pid) in self.m_marker_data if pid == -1]
            if len(root_tstamps) > 0:
                begin_timestamp = min([t for (t, _) in root_tstamps])
                end_timestamp = max([t for (_, t) in root_tstamps])
                plot_idx = (self.m_curr_tick % self.m_plot_capacity)
                self.m_gpu_plot_data[plot_idx][0] = self.m_curr_tick
                self.m_gpu_plot_data[plot_idx][1] = (end_timestamp - begin_timestamp) * 1000
            self.m_curr_tick = self.m_curr_tick + 1
        
