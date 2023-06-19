
Host Programming incorporates two steps procedure for delivery of new software to CC31xx embedded device.
The 1st step is formatting of the serial flash. This step is optional.
The 2nd step is the ServicePack programming.

Revision: 1.3.0 (dated August 2017)


Limitation / Bugs
-----------------
- to enable flash formatting from host, SimpleLink driver needs to be compiled with FORMAT_ENABLE.
  For instructions, please follow Host Programming.pdf under \docs\examples

Additional Documents
--------------------
docs\examples\host_programming.pdf

Pre-requisite
-------------
1. CC3100 BoosterPack with PG1.33 device.
2. MSP430F5529 Launchpad
3. CC3100 SDK 1.3.0
4. Latest CC3100 ServicePack
5. CCS v7
