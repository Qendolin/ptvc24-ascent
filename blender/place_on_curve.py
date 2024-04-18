import bpy
from bpy.types import WorkSpaceTool
from mathutils import Vector, Matrix
from bpy_extras import view3d_utils 

bl_info = {
    "name": "Place on Curve",
    "blender": (4, 0, 0),
    "category": "Object",
}

class PlaceOnCurveTool(WorkSpaceTool):
    bl_space_type = 'VIEW_3D'
    bl_context_mode = 'OBJECT'

    # The prefix of the idname should be your add-on name.
    bl_idname = "qendolin.place_on_curve_tool"
    bl_label = "Place on Curve"
    bl_description = (
        "Snap objects to curves."
        "Aligns y axis to tangent."
    )
    bl_icon = "ops.transform.translate"
    bl_widget = "VIEW3D_GGT_tool_generic_handle_free"
    bl_widget_properties = [
        ("radius", 50.0),
        ("backdrop_fill_alpha", 0.0),
    ]
    bl_keymap = (
        ('view3d.select', {'type': 'LEFTMOUSE', 'value': 'CLICK'}, {'properties': [('deselect_all', True)]}), 
        ("qendolin.place_on_curve_operator", {"type": 'LEFTMOUSE', "value": 'CLICK_DRAG'}, None),
    )

    def draw_settings(context, layout, tool):
        pass

def getPtFromT(p0, p1, p2, p3, t):
    c = (1 - t)
    pt = (c ** 3) * p0 + 3 * (c ** 2) * t * p1 + \
        3 * c * (t ** 2) * p2 + (t ** 3) * p3
    return pt

def getTangentAtT(p0, p1, p2, p3, t):
    c = (1 - t)
    tangent = -3 * (c * c) * p0 + 3 * c * c * p1 - 6 * t * c * p1 - \
        3 * t * t * p2 + 6 * t * c * p2 + 3 * t * t * p3
    return tangent

class PlaceOnCurveOperator(bpy.types.Operator):
    """Tooltip"""
    bl_idname = "qendolin.place_on_curve_operator"
    bl_label = "Place on Curve"

    @classmethod
    def poll(cls, context):
        return context.active_object is not None

    def execute(self, context):
        active_obj = context.active_object
        if active_obj is None:
            self.report({'ERROR'}, "No active object.")
            return {'CANCELED'}
        
        target_positon = self._initial_location + self._offset
        closest_spline = None
        min_distance = float('inf')
        for obj in context.scene.objects:
            if obj == active_obj:
                continue
            if obj.type == 'CURVE':
                for spline in obj.data.splines:
                    if spline.type == 'BEZIER':
                        for p in spline.bezier_points:
                            distance = (target_positon - obj.matrix_world @ p.co).length
                            if distance < min_distance:
                                min_distance = distance
                                closest_spline = spline
        
        if closest_spline is None:
            print("No spline")
            return {'CANCELED'}

        closest_point = None
        tangent = None
        min_distance = float('inf')
        if len(spline.bezier_points) >= 2:
            r = spline.resolution_u + 1
            segments = len(spline.bezier_points)
            if not spline.use_cyclic_u:
                segments -= 1

            for i in range(segments):
                inext = (i + 1) % len(spline.bezier_points)

                knot1 = spline.bezier_points[i].co
                handle1 = spline.bezier_points[i].handle_right
                handle2 = spline.bezier_points[inext].handle_left
                knot2 = spline.bezier_points[inext].co

                for i in range(r):
                    P = getPtFromT(knot1, handle1, handle2, knot2, i / r)
                    distance = (target_positon - P).length
                    if distance < min_distance:
                        min_distance = distance
                        closest_point = P
                        tangent = getTangentAtT(knot1, handle1, handle2, knot2, i / r)

        up = Vector((0, 0, 1))
        T = tangent.normalized()
        N = tangent.cross(up.cross(T).normalized())
        B = T.cross(N)
        TBN = Matrix((B, T, N)) # B & T intentionally swapped
        TBN.transpose()
        Q = TBN.to_quaternion()

        active_obj.location = closest_point.copy()
        active_obj.rotation_mode='XYZ'
        active_obj.rotation_euler = Q.to_euler('XYZ')

        return {'FINISHED'}
    
    def mousePos3d(self, context, event):
        mouse_pos = event.mouse_region_x, event.mouse_region_y
        region = context.region
        region3d = context.space_data.region_3d
        view_vector = view3d_utils.region_2d_to_vector_3d(region, region3d, mouse_pos)
        world_loc = view3d_utils.region_2d_to_location_3d(region, region3d, mouse_pos, view_vector)
        return world_loc
    
    def modal(self, context, event):
        if event.type == 'MOUSEMOVE':
            self._offset =  self.mousePos3d(context, event) - self._initial_mouse
            self.execute(context)
            context.area.header_text_set("Offset %.4f %.4f %.4f" % tuple(self._offset))

        elif event.type == 'LEFTMOUSE':
            context.area.header_text_set(None)
            return {'FINISHED'}

        elif event.type in {'RIGHTMOUSE', 'ESC'}:
            context.active_object.location = self._initial_location
            context.area.header_text_set(None)
            return {'CANCELLED'}

        return {'RUNNING_MODAL'}
    
    def invoke(self, context, event):
        if context.space_data.type == 'VIEW_3D':
            v3d = context.space_data
            rv3d = v3d.region_3d

            if rv3d.view_perspective == 'CAMERA':
                rv3d.view_perspective = 'PERSP'

            self._initial_mouse = self.mousePos3d(context, event)
            self._initial_location = context.active_object.location.copy()

            context.window_manager.modal_handler_add(self)
            return {'RUNNING_MODAL'}
        else:
            self.report({'WARNING'}, "Active space must be a View3d")
            return {'CANCELLED'}


def register():
    bpy.utils.register_class(PlaceOnCurveOperator)
    bpy.utils.register_tool(PlaceOnCurveTool, after={"builtin.scale_cage"}, separator=True, group=True)


def unregister():
    bpy.utils.unregister_tool(PlaceOnCurveTool)
    bpy.utils.unregister_class(PlaceOnCurveOperator)



if __name__ == "__main__":
    register()
