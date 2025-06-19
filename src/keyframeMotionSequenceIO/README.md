# Exporting Joint Angle Time Series Data Out of Maya

This utility I/O plugin can be used to export joint angles out of Maya. Joint angles are exported in JSON format, although the extension of "kmexp" is used to prevent extension conflicts with the <a href="https://github.com/lakshmipathyarjun6/kinematic-motion-retargeting/tree/main/src/contactSequenceIO">contactSequenceIO</a> plugin (Maya only allows one extension per I/O plugin).

## Usage

1. Set the frame range in the animation timeline to the minimum and maximum frame you want to export. The plugin will automatically export the entire range because it is not possible to specify a frame range otherwise. For example, if you have keyframes from frames 1 - 703, but your timeline is set to 1 - 800, then you will get 800 frames of data instead of 703.

2. Open the "Export All" menu under File and change the filetype to "kmexp".

3. Hit the export button. If export was successful, a fully populated JSON file (but using the kmexp extension) should be generated at the filepath specified. Exports follow the schema below:

{
    "framerate": 120 (all GRAB exports are at 120 fps),
    "jointNames": [
        "name_of_joint_0_according_to_URDF",
        "name_of_joint_1_according_to_URDF",
        ....
    ],
    "jointDofs": [
        number_of_dofs_in_joint_0,
        number_of_dofs_in_joint_1,
       ...
    ],
    "table": [
        scale_x,
        scale_y,
        scale_z,
        qrot_w,
        qrot_x,
        qrot_y,
        qrot_z,
        trans_x,
        trans_y,
        trans_z
    ],
    "data": [
        {
            "frame": frame_number,
            "object": [
                  qrot_w,
                  qrot_x,
                  qrot_y,
                  qrot_z,
                  trans_x,
                  trans_y,
                  trans_z
            ],
            "hand": [
                  qrot_w,
                  qrot_x,
                  qrot_y,
                  qrot_z,
                  trans_x,
                  trans_y,
                  trans_z
                  dof_1_angle,
                  dof_2_angle,
                  ....
            ]
        },
        ...
   ]
}

The root dof will always have 7 DOF values, with the first 4 elements being the orientation expressed as a quaternion in wxyz order and the second 3 being the translation. I realize it's not quite accurate to declare the quaternion values all as DOFs, but it is sufficient for parsing purposes and certainly better than Euler angles. Joint angles values are all in radians, and translation and scale values are in centimeters (assuming Maya default units were not changed).
