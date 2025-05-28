# Exporting and Importing Data from the GRAB Dataset

This plugin imports data from the <a href="https://grab.is.tue.mpg.de/">GRAB dataset</a>, which serves as the source for all our original motions, into Maya.

## Getting started

Download the dataset to get started. Please note that we do not own the rights to this dataset and are unable to host any original content. By downloading this dataset, you agree to all the terms and conditions set forth by GRAB's license.

You will also need to download (and agree to the terms and conditions of) the MANO hand model, which is available <a href="https://mano.is.tue.mpg.de/">here</a>.

Finally, please clone <a href="https://github.com/lakshmipathyarjun6/GRAB">my fork of the GRAB processing repository</a>.

## Exporting original GRAB data

Because processing raw GRAB npz files requires the use of custom Python libraries, we need to convert the raw data into a format which is entirely self-contained and can be loaded into a third-party viewer (e.g. Maya).
