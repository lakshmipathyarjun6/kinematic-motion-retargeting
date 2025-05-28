# Kinematic Motion Retargeting for Contact-Rich Anthropomorphic Manipulations 

Code for paper https://dl.acm.org/doi/10.1145/3723872

# Introduction

This code is intended to be executed as a collection of plugins for Autodesk Maya. If you are unfamiliar Maya, there are a number of YouTube tutorials on getting started. We don't assume any prior knowledge beyond knowing how to navigate the viewport and a basic understanding of the default toolset. The code itself is primarily written in C++, is built with CMake, and requires the C++ 20 standard or higher. There are also some additional Python and MEL (Maya scripting language) scripts that are used for utility purposes. This codebase is intended to walk users through the process of creating results for *new hands* from scratch or, alternatively, creating retargeted motions for *new motions* for one of our existing hands.

For those who are only looking to download complete results for baseline comparisons or downstream applications, I strongly suggest just downloading the Maya binary files directly from [TODO: here]. If you want to export motions for use in other applications (e.g. keyframes, final B-splines, etc.), please skip down to the export section.

# Why on earth are you using Maya instead of literally anything else (e.g. Blender, Open3d, \<insert favorite Python visualizer\>)?

Because I had to familiarize myself with it during the development of this paper: https://dl.acm.org/doi/10.1145/3592117. Yes, I realize Maya is closed source. Yes, I realize Maya is cumbersome to build on Linux distributions and has some problems. But it does have a rather nice UI and, more importantly, a well documented C++ developer API [https://help.autodesk.com/view/MAYAUL/2024/ENU/?guid=MAYA_API_REF_cpp_ref_group_open_maya_html]. As the sole developer of this work, I opted to use what was most useful and convenient for my research needs. However, I am more than happy to provide guidance if someone is interested in porting this code to Blender or a Python package. But I personally do not have the bandwidth to do so.

# Installation

You will first need to download and install Autodesk Maya with a valid license key.

If you do not already have CMake installed, please do so as well.

Next, install Eigen[ref]. The easiest way to do so is via a package manager (Hombrew on Mac, apt on Linux, Chocolatey or similar on Windows).

Next, clone this repository *recursively* to make sure all dependency submodules are fetched:

```
git clone --recursive https://github.com/lakshmipathyarjun6/kinematic_motion_retargeting.git
cd kinematic_motion_retargeting/
```

You will then need to change the Maya version in the CMake file [here] to whatever year / version of Maya you installed on your machine. The default CMake file assumes 2025. This code should be foward and backwards compatible by at least a few years, but please let me know if you encounter problems with your version so I can flag the earliest / latest compatible versions.

You can now build the CMake project:

```
mkdir build
cd build/
cmake ..
make -j5
```

If successfull, you should see a number of dynamic libraries (.bundle on Mac, .dylib on Windows, .so on Linux) generated in the build folder.

