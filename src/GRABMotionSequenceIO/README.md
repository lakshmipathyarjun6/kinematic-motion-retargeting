# Exporting and Importing Data from the GRAB Dataset

This plugin imports data from the <a href="https://grab.is.tue.mpg.de/">GRAB dataset</a>, which serves as the source for all our original motions, into Maya.

## Getting started

Download and unpack the dataset to get started. Please note that we do not own the rights to this dataset and are unable to host any original content. By downloading this dataset, you agree to all the terms and conditions set forth by GRAB's license.

You will also need to download (and agree to the terms and conditions of) the MANO hand model, which is available <a href="https://mano.is.tue.mpg.de/">here</a>. Unzip and rename the downloaded folder to just 'mano', then move the MANO_LEFT and MANO_RIGHT pkl files under the 'models' subfolder to the top-level folder (e.g. your path should be mano/MANO_RIGHT.pkl). 

Finally, please clone <a href="https://github.com/lakshmipathyarjun6/GRAB">my fork of the GRAB processing repository</a>.

## Exporting original GRAB data

Because processing raw GRAB npz files requires the use of custom Python libraries, we need to convert the raw data into a format which is entirely self-contained and can be loaded into a third-party viewer (e.g. Maya). Create a conda or other virtual environment and install any packages required by the GRAB code repository.

Next, create a destination folder for any data exports.

If all is set up correctly, you should be able to run the following:

```
cd GRAB/grab
python exportRhandFullSequence.py --grab-path <PATH_TO_GRAB_DATASET_FOLDER> --out-path <PATH_TO_YOUR_EXPORT_FOLDER> --model-path <PATH_TO_YOUR_MANO_FOLDER> --motion-path <PATH_TO_GRAB_MOTION_YOU_WANT_TO_EXPORT>
```
