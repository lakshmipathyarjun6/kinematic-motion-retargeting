# Motion Retargeting Pipeline

This Edit Context plugin can be thought of as the "main" plugin which actually performs the motion retargeting.

## Setup

WARNING: There are significantly more steps for this plugin than the others. Since there is a lot to write out in text, I have made a [TODO: video] explaining how to set up and use the plugin.

As a starting point, it assumes you have:

1. A Maya scene with paired original hand and object contacts computed using the <a href="https://github.com/lakshmipathyarjun6/kinematic-motion-retargeting/tree/main/src/contactRaytraceContext">contactRaytraceContext</a> plugin. 

2. Contact data for the target hand in a JSON file exported from the <a href="https://github.com/lakshmipathyarjun6/kinematic-motion-retargeting/tree/main/src/contactTransferEditContext">contactTransferEditContext</a> plugin using the <a href="https://github.com/lakshmipathyarjun6/kinematic-motion-retargeting/tree/main/src/contactSequenceIO">contactSequenceIO</a> plugin.

Next, copy the MEL files under the /scripts subdirectory to the following location:

Mac:
```
cp path/to/project/kinematic-motion-retargeting/src/fusedMotionEditContext/scripts/* ~/Library/Preferences/Autodesk/maya/<year>/scripts
```

Linux:
```
cp path/to/project/kinematic-motion-retargeting/src/fusedMotionEditContext/scripts/* ~/maya/<year>/scripts
```

Windows:
```
TODO
```

If you have the previous two items ready to go and have copied the scripts over, the [TODO: video] will guide you through the remainder of the process.

## Plugin activation

Run the following MEL command to activate the plugin:
```
setToolTo fmec
```

If successfull, you should see the following Tool Editor menu:

<TODO: TW Image>

## Plugin tools

<TODO: Tool explanations>

## General workflow

Please see the [TODO: video] for detailed steps. But as a rough guideline:

1. Compute the trajectory using only root degrees of freedom (DOFs).

2. Compute the trajectory using all (DOFs). This process can be very slow, especially if there are a lot of contacts. I would strongly suggest letting it run overnight or all day. The root cause of this slowdown is that all gradients are computed using finite differencing - please see the paper for more details.

3. Filter the trajectory using low pass and peak removal filters.

4. Re-compute the trajectory of after filtering with all DOFs included.

5. Run acceleration refinement to improve temporal consistency.

6. Save the final result, then close and re-open Maya.

Next (and final) step: <a href="https://github.com/lakshmipathyarjun6/kinematic-motion-retargeting/tree/main/src/smoothMotionEditContext">smoothMotionEditContext</a>
