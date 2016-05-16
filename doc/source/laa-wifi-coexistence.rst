LAA Wi-Fi Coexistence Module Documentation
-----------------------------------------

.. include:: replace.txt


.. heading hierarchy:
   ------------- Document Title
   ************* Chapter (#)
   ============= Section (#.#)
   ############# Subsection (#.#.#)
   ~~~~~~~~~~~~~ Paragraph (no number)


Introduction
************

The LAA Wi-Fi Coexistence module provides support to simulate with |ns3| the scenarios for
the coexistence between unlicensed variants of LTE in the 5 GHz band and Wi-Fi 
systems, following the description in [TR36889]_. 

This module is named the 'LAA' coexistence module, where LAA stands for
Licensed-Assisted Access (LAA).  We wish to clarify that 
as of this writing, there are different proposals in the 3GPP standardization
process as to what would constitute the Licensed-Assisted Access (LAA), and
that other terms in use include LTE-Unlicensed or just simply unlicensed LTE.
This module is presently supporting a generic duty-cycled unlicensed LTE
network coexisting in the lower U-NII Wi-Fi bands (5.15-5.25 GHz), and
not a specific LAA standardization proposal; however, the capabilities
could be extended for specific LAA variants in the future.  We have selected
the acronym 'LTE-DC' (for LTE duty cycle) to refer to the model currently
implemented.

The support for simulating the LTE and Wi-Fi technology is provided by the
separate |ns3| LTE and Wi-Fi modules; the LAA Wi-Fi Coexistence module focuses on the
additional features that are needed to create a simulation scenario
that merges the two technologies. The source code for the LAA Wi-Fi Coexistence
module lives in the directory ``src/laa-wifi-coexistence``. 

The LTE and Wi-Fi modules of |ns3|, as stand-alone modules, are separately 
documented in other documents.  This document primarily concerns itself
with extensions and tests to support coexistence scenarios.
Over time, extensions found in this module may migrate to the existing
|ns3| main development tree.

The rest of this document is organized into five major chapters:

2. **Design:**  Describes how |ns3| has been extended to support coexistence
   studies, and describes scenario design.
3. **Usage:**  Documents how users may run and extend these simulation 
   scenarios themselves.
4. **Validation:**  Documents how the models and scenarios have been verified
   and validated by test programs.
5. **Open Issues and Future Work:**  Describes topics for which future work
   on model or scenario enhancements is recommended, or for which questions
   on interpretations of standards documents may be listed.

Design
******

In the following, we document the design decisions and modeling
assumptions on which the LAA Wi-Fi Coexistence module is based.

|ns3| or any performance modeling tool will be able to satisfy some
or all of the desired aspects of the scenario definitions.  We summarize
herein the requirements driving this design, and then go into greater
detail about the scope and limitations of the |ns3|-based framework. 


Use case requirements
=====================

This module is intended to facilitate coexistence performance evaluations
as described in [TR36889]_.  To summarize, [TR36889]_ describes indoor
and outdoor scenarios that may be parameterized and instrumented to
study questions of fairness and performance impacts of LAA services with 
respect to Wi-Fi.

In summary, this module intends to extend |ns3| to support the modeling
of deployment scenarios, summarized in Section 6 of [TR36889]_, involving
deployment of multiple system operators (Wi-Fi and LAA) in the same
radio region.  Annex A of [TR36889]_ further outlines desired parameters
for scenario configuration, system configuration, and performance metrics.

Rather than restating the requirements herein, we suffice to state that
the target is to support simulation scenarios as outlined in Section 6
and Annex A of [TR36889]_.

Packet-level simulation
=======================

Simulation tools for network performance evaluation differ on the scope,
or level of abstraction, for simulations.  Tradeoffs exist between increasing
fidelity in particular aspects of the system, and reducing resource 
requirements of the simulation, in terms of memory usage, computational
time, and wall-clock runtime necessary to conduct the simulation.  
On the one hand, this may mean that particular tools are unsuitable to
answer specific questions.   A tool that abstracts the physical layer
properties of the system too much will not be useful to study subtleties
of the design or performance of the physical layer.  However, a tool
that provides high fidelity of the physical layer, down to the symbol
level with ray tracing propagation models, will not be useful to study
large-scale wireless questions such as the interference properties of 
thousands of nodes coexisting in a stadium or arena.  

Network simulators are sometimes categorized as being physical-layer
simulators, packet-level simulators, and session-level simulators.  One
way to think of this is to think of the granularity of modeling fidelity
as being at the symbol, (sub)carrier, or propagation ray level for
physical-layer simulators, at the frame or packet level for packet-level
simulators, and at the flow or session level (i.e. individual packets are
not modelled) for session-level simulators. In the following, we
clarify how these concept apply to the |ns3| Wi-Fi and LTE models.

Wi-Fi model granularity
#######################

The |ns3| Wi-Fi model is a packet-level simulation model.
This means that ns-3 provides 
modeling granularity down to the PDU level at the boundary of the
MAC and PHY, but the PDUs are not further decomposed into bits or
symbols such as in an actual implementation.  

In |ns3|, physical layer effects are abstracted and accounted for by
the modeling assumptions in the channel and the receiver.  |ns3| Wi-Fi Packets
(logically, a Wi-Fi frame but encapsulated in an ns3::Packet data structure)
are sent through |ns3| Channel objects that schedule the Packet for
arrival on a receiver attached to the channel; the receive time is 
scheduled based on the transmission delay (data rate) and propagation delay
between the two stations.  These Packets contain
information about the assumed modulation and coding rate used on
the channel, as well as a value to track the signal power, but the channel
and receiver model the physical layer effects by adjusting the signal
and noise levels according to statistical loss models and abstractions of
antennas.  When it comes time to decide whether a frame is received, the
signal-to-noise/interference levels are used to lookup error rates 
corresponding to the modulation and coding being modelled (and possibly
the antenna configuration and other effects) to make decisions such as
"Was the preamble decoded and sync obtained?" or "Was the payload received
error-free?".

Above the physical layer, the Wi-Fi MAC is modeled with relatively high
accuracy, since the unit of granularity in |ns3| (the frame, in this case)
is the same unit of granularity dealt with in actual implementations and
in the standard.

Physical layer simulators can be used to provide a number of error-rate
tables used in simulators such as |ns3|, through a technique known
as ``link-to-system-mapping``, but the key thing to understand is that,
at the physical layer, everything in the modeled system is oriented towards
obtaining an estimate of a received frame's SINR, or accounting for 
signal energy in the system for CCA modeling.  The frame's SINR may 
then be used to decide on the probability of successful reception, and 
above this MAC/PHY boundary, the |ns3| modeling granularity permits 
a fuller modeling.

One research group developed a higher fidelity Wi-Fi physical layer simulator
for |ns3|, called ``PhySimWifi`` [PhySimWifi]_.  However, this has not been
separately maintained or adopted by the main |ns3| project, partly because
it takes quite a long time to run simulations of even the simplest
configurations.


LTE model granularity
#####################

The |ns3| LTE model is a  simulation model that has a granularity
corresponding to a Resource Block (RB) on the frequency domain and to
the LTE subframe structure in the time domain. In LTE, a RB is the
unit used for the allocation of radio resources, corresponding to a
set of 12 subcarriers (180 kHz total). According to the standard
[TS36211]_, the downlink control frame starts at the beginning of each
subframe and lasts up to three symbols across the whole system
bandwidth, where the actual duration is provided by the Physical
Control Format Indicator Channel (PCFICH). The information on the
allocation are then mapped in the remaining resource up to the
duration defined by the PCFICH, in the so called Physical Downlink
Control Channel (PDCCH). A PDCCH transports a single message called
Downlink Control Information (DCI) coming from the MAC layer, where
the scheduler indicates the resource allocation for a specific user. 
The PCFICH and PDCCH are modeled with the transmission of the control
frame of a fixed duration of 3/14 of milliseconds spanning in the
whole available bandwidth, since the scheduler does not estimate the
size of the control region. This implies that a single transmission
event models the entire control frame with a fixed power (i.e., the
one used for the PDSCH) across all the available RBs. The same
simulated event is also used to model the DL Reference Signal (RS). 
The Sounding Reference Signal (SRS) transmitted in UL are modeled
similar to the downlink control frame. The SRS is periodically placed
in the last symbol of the subframe in the whole system bandwidth.  

The |ns3| LTE model implementation uses the Spectrum module to
represent the power spectral densities of transmission events with the
RB granularity and to associate to these events other data structures
specific of the LTE model that are used to simulate other PHY and MAC
aspects, such as lists of |ns3| Packets modeling the MAC PDUs being
transmitted, and other structures for conveying DCI and other LTE
control messages.

Support for mixed signal simulations
====================================

As of the most recent |ns3| release, the physical layer frameworks for 
LTE and Wi-Fi models were different.  Wi-Fi was one of the earliest 
models in |ns3|, and the physical layer implemented involves the use
of strongly typed channels and network devices.  In |ns3|, all nodes
have a "NetDevice" object that is connected to a "Channel" object.

To date, the |ns3| `wifi` module has not supported mixed-signal
simulations.  The physical layer design originated from a prototype
simulator known as `Yans`, in which the interconnection between devices
and logical channels was strongly typed; i.e. only a Wi-Fi device 
could be connected to a Wi-Fi channel, or else there would be a 
compilation error.  As a result, it has not been possible to inject
foreign signals such as LTE into the Wi-Fi receiver.  Furthermore,
the physical layer in the Yans model does not support modeling
of frequency-selective effects.

The Spectrum framework ([BalMio09]_) was introduced into ns-3 around 2009, 
and most new wireless models have used this newer framework, including LTE.  
The distinguishing features of this framework are that the channel model
is technology independent, and that signals have a frequency-domain
representation as well as a time-domain representation.  With regard
to the first point, what this means is that transmitters emitting signals
with different power spectral densities, according to different technologies,
can coexist and be received by any device.  The main implication of this
is that each technology needs to have a model for how to handle a 
non-native transmission (e.g. either integrated as wideband noise
across the received channel, or handled in a technology-specific way).
The second feature allows for frequency-selective effects to be modeled
in the simulator, whether from frequency-selective fading or from issues
such as adjacent channel interference due to imperfect transmit or
receive filters on a channel.  The signals are not represented down to
the symbol level, but rather, the signal spectrum is divided up into
frequency bands, to which a portion of the overall signal power is allocated.
For a multi-carrier system like OFDM, what this means is that loss models
may reduce the received power in a frequency band more or less than other
bands if frequency selective fading is included.  A receive error model
(not at a symbol level but at some higher level of abstraction) must still be 
able to deal with the received signal (represented as power in different
bands) in a meaningful way.  

The main implication of using this framework is that receiver error models 
must be developed to account for the different signal structure and for
coexisting signals.  It is possible to
reuse existing (non-frequency-selective) propagation loss models with
the Spectrum framework, but some Spectrum-aware (i.e. frequency-selective)
fading models are also available.  In general, use of spectral decomposition
of signals also incurs runtime performance overhead since the computations
required to model a reception are more involved.  Therefore, the legacy
model is still of benefit for situations that do not need to model 
frequency-selective effects or coexistence between technologies.  

For this project, however, Spectrum features are desired, and so we introduce
the new SpectrumWifiPhy that has been developed in the next section.
 
class SpectrumWifiPhy
====================

This section describes the implementation of the ``SpectrumWifiPhy``
class that can be found in ``src/wifi/model/spectrum-wifi-phy.{cc,h}``.

The implementation also makes use of additional classes found in the
same directory:

* ``wifi-spectrum-helper.{cc,h}``
* ``wifi-spectrum-phy-interface.{cc,h}``
* ``wifi-spectrum-signal-parameters.{cc,h}``

A new physical layer model has been built based on the |ns3| Spectrum
framework.  This has resulted in a ``SpectrumWifiPhy`` class that
reuses the existing interference manager and error rate models, but
that allows, as a first step, foreign signals to be added to the
noise on the channel and treated as noise.

There are three key classes responsible for deciding whether an |ns3|
Wi-Fi frame is successfully received at the physical layer.  The first
class is the physical layer model itself, which historically has been
the class ``YansWifiPhy`` but which can now be substituted by
``SpectrumWifiPhy``.  These ``Phy`` classes implement the 'transmit'
and 'receive' methods and interact with the channel.

The other two classes supporting the Phy on the receive side 
are the ``InterferenceHelper`` and the ``ErrorRateModel``, the latter
of which there are two similar ones in |ns3| (the 
``YansErrorRateModel`` and the ``NistErrorRateModel``).  Figure 
:ref:`fig-legacy-snir` provides an overview of the operation.
The ``InterferenceHelper`` is responsible for tracking all signals
including their receive power, duration, and attributes about the
details of the wifi signal itself (e.g. preamble, mode).  This class
can be thought of as a bookkeeper that adds signals (|ns3| Packets) 
to the collection.  The physical layer decides (based on the state
of the channel as represented in this tracker) whether it can try
to successfully receive a frame.  It tells the InterferenceHelper
about this primary (intended to receive) packet, and schedules
an 'EndReceive' event for the packet.  By the end of this packet
reception, other frames may have arrived as shown in the figure.
The InterferenceHelper breaks the time interval spanning the
primary packet to be received into "chunks", each with a different
Signal to Interference + Noise (SINR) ratio.  This chunk of bits
is then evaluated by looking up the probability of successful
reception corresponding to the mode (modulation) in use.  The
|ns3| random number generator is then used to pick a random variate
and decide whether the chunk was successfully received or not.  If
all such chunks are successfully received, then the physical layer
accepts the packet as a successful receive.

.. _fig-legacy-snir:

.. figure:: figures/legacy-snir.*
   :align: center

   Conceptual model of the legacy |ns3| interference model

Note that until now, the above classes could only handle signals
sent on Wifi channels by ``YansWifiPhy`` devices.  We decided
in this implementation to reuse the existing InterferenceHelper
and ErrorRateModel classes and adapt the Spectrum-based signals
to those classes.    Two main changes are needed to facilitate this.
First, the physical layer must send signals compatible with the
Spectrum channel framework, and in particular, the 
``MultiModelSpectrumChannel`` that allows signals from different
technologies to coexist.  Second, the InterferenceHelper must be
extended to support the insertion of non-Wi-Fi signals and to
add their received power to the noise, in the same way that
unintended Wi-Fi signals (perhaps from a different SSID or arriving
late from a hidden node) are added to the noise.

As a result, the ``SpectrumWifiPhy`` transmit and receive methods
were adapted to use the Spectrum channel API.  This required developing
a few ``SpectrumModel``-related classes.  The class
``WifiSpectrumHelper`` is used to create Wi-Fi signals with the
spectrum framework and spread their energy across the bands.  For
this initial implementation, the entire 5GHz ISM band is modeled,
split into 5MHz-wide bands.  The power allocated to a particular channel
is spread across +/- 10MHz (four) bands centered on the channel center
frequency; this is for a 20 MHz signal.  This could easily be extended
to provide more granularity (e.g. 1MHz bands) and to place power outside
of the channel according to the real spectral mask.  This should be
done for future adjacent channel models, but for the current co-channel
effort, this was not added.  A simple model was used since the simpler
the model, the more computationally efficient it is, and we are not
yet using the additional wider signals.  Similarly, on the receive
side, a receiver filter mask can be defined; for this initial
implementation, we implemented a perfect brick wall filter that is 
20 MHz wide centered on the channel center frequency.

The current scripts for coexistence support channel 36 at 5.180 GHz only,
but this restriction could be removed in the
future once a network plan for the scenarios, requiring multiple
channels, is developed.  Specifically, there is initial support added
as part of this project for multiple channels (36, 40, 44, 48) and roaming, 
but it is not enabled in the scenario scripts.

To support an easier user configuration experience, the existing
YansWifi helper classes (in ``src/wifi/helper``) were copied and
adapted to provide equivalent SpectrumWifi helper classes.

Finally, for reasons related to avoiding C++ multiple inheritance
issues, a small forwarding class called ``WifiSpectrumPhyInterface``
was inserted as a shim between the ``SpectrumWifiPhy`` and the
Spectrum channel.

Enhancements to clear-channel access (CCA)
==========================================
Extensions have been added to the receive packet processing to support
the requirements for CCA-energy detect (CCA-ED) and CCA-carrier sense
(CCA-CS) thresholds. In support of these modifications, a new reception
stage was introduced to model the probability of successful frame
synchronization as a function of the signal's SINR.

To briefly summarize, the legacy YansWifiPhy did not explicitly model
frame synchronization, and did not have CCA thresholds corresponding to
the -62 dBm (CCA-ED) and -82 dBm (CCA-CS) thresholds for 802.11n found
in the 2012 standard. Furthermore, the legacy CCA was not able to sense
non-Wi-Fi signals for energy detection. Instead, it worked as follows.
When the start of a Wi-Fi frame arrived at a receiver, if the receiving
node was not already in TX, RX, or SLEEP state, it compared the RX frame
power with an 'energy detection threshold' defaulting to -96 dBm. If
above this threshold, frame synchronization was deemed successful and a
frame RX event was scheduled. If not, the aggregate channel energy (from
the aforementioned RX frame and any other frames in the channel) at the
receiver was compared to the 'CcaMode1Threshold', which defaulted to
-99 dBm. If higher than the threshold, the receiving node's PHY state was
set to CCA_BUSY, deeming the channel as busy for the duration that the
channel energy was above it.

The new CCA enhancements more accurately model real systems and complies
with the standard. An updated algorithm is implemented as follows. When a
signal (Wi-Fi, LTE, or other) arrives at a Wi-Fi receiver, if its PHY is
in IDLE or CCA_BUSY state (not RX, TX, or SLEEP), Wi-Fi frame time
synchronization is attempted. This frame sync always fails if the incoming
signal is not Wi-Fi. If it is a Wi-Fi signal, a probabilistic draw is done
to determine if the frame sync is successful or not based on the frame error
rate corresponding to the average SINR of the RX frame during its PLCP SYNC
field. The ``FrameSyncErrorRateLookup`` class was added to provide a lookup
table for SINR to frame sync error rate. A frame RX event is scheduled if
frame sync is successful. If unsuccessful, the aggregate channel energy
(all energy in the channel from any source) is compared the the
'CcaMode1Threshold', whose default value is changed to -62 dBm to reflect
the CCA-ED threshold defined in the standard. The receiving node's PHY state
is set to CCA_BUSY for the duration that the channel energy is above the
threshold. A flowchart summarizing the updated CCA functionality is
displayed in figure :ref:`fig-wifi-cca-flowchart`. The RX sensitivity threshold
shown in the flowchart has not yet been implemented---currently frame sync
is attempted at any energy level.

.. _fig-wifi-cca-flowchart:

.. figure:: figures/wifi-cca-flowchart.*
   :align: center

   Flowchart of the updated CCA implementation

To support the frame sync model described above, the University of Washington
(UW) Fundamentals of Networking Laboratory's Wi-Fi link simulator was used
to generate error rate tables for both an AWGN channel and a frequency
selective channel corresponding to IEEE 802.11 TGn's Channel Model D
[TGnChannelModels]_. The link simulator models a Wi-Fi TX to RX link and
uses the delayed autocorrelation method detailed in [DelayedAutocorrSync]_
for frame time synchronization. An option is provided in |ns3| for users to select
between AWGN or Channel Model D for frame sync error probabilities. The
link simulation frame sync results for both channels, shown in figure :ref:`fig-uw-linksim-ser`, exceed the
standard-specified CCA-CS threshold of Wi-Fi frame detection greater
than 90% probability at or above -82 dBm. When retrieving the frame sync error rate
for a given SNR in |ns3|, linear interpolation is applied if the SNR falls between the mapped link simulation datapoints.

.. _fig-uw-linksim-ser:

.. figure:: figures/frame_sync_error_rate.*
   :align: center

   UW link simulator frame synchronization error rates

Future work in this area includes implementing the RX sensitivity threshold,
which corresponds to the minimum aggregate energy at a Wi-Fi receiver for
its receive hardware to be enabled for frame synchronization and reception.
In addition, an API is planned to enable users to add their own mapping of
SINR to frame sync error rate to be used in the CCA model.

Enhancements to RSS-based AP selection
======================================

Access point selection in |ns3| is based on passive or active scanning
of the channel to which the node is attached.  The state machine supports
association and disassociation (when too many beacons are missed) but does
not implement received signal strength (RSS) AP selection, nor scanning
across channels.  

This project has extended the MAC state machine for STA nodes by
allowing RSS-based AP selection; with the extension, STA nodes can
scan across the set of channels supported by the physical layer, and
will "dwell" on each channel, retaining memory of the best-seen
beacon (or probe response), and then initiate an association with
the best found AP on its network (SSID) once the scanning process 
completes.  Regardless of whether passive- or active-scanning is 
configured, a STA will dwell on each channel for a "ScanningTimeout" (default 
value of 120 ms) before moving to the next channel or making a selection.  

To fully implement seamless roaming across channels requires a model for 
dynamically handling handovers at the IP layer, or for dynamically 
configuring tunnels to manage the changes in access points.  For the
current project, a general dynamic distributed protocol to manage 
topology changes has not been implemented; instead, roaming is handled
by dynamically inserting routing table entries in both the traffic 
source and the STA nodes upon successful authentication.  Specifically,
an trace source named 'Assoc' in the Wifi MAC layer is triggered whenever
a station successfully associates.  A callback function is registered with
this trace source, and when it is called, the callback function inserts
a default route via the new AP in the STA's routing table, and inserts a
host route (via the new AP) in the traffic source node's routing table.

Enhancements to MIMO modeling 
=============================

Networks based on 802.11n and newer variants, in practice, are deployed with 
multiple antennas on either the stations or access points or both.  
These multiple antennas can be used either for diversity (improving the 
reception probability for a given signal-to-noise/interference ratio) or 
for spatial multiplexing (to increase the data rate).  Receiver diversity 
(i.e. adding multiple antennas at the receiver) can improve the signal to
noise ratio, and transmit diversity approaches via Space Time Block Codes 
such as Alamouti codes can also contribute gains.  Rate enhancements are 
achieved in MIMO via spatial multiplexing schemes at the transmitter to 
create effective 'parallel' streams between transmitter and receiver along 
with suitable receiver decoding. 

802.11n-based systems support a number of different maximum data rates 
depending on combinations of system parameters such as modulation and
coding schemes, number of spatial multiplexing streams, whether or not 
short guard intervals are used, and the bandwidth of the channel(s) used.
Since |ns3| is a packet-level simulator, a key factor in the ability of
|ns3| to support the model of a particular mode of operation is whether
or not a suitable error model exists for the mode.

The most recent release of |ns3| supports the following modes 
(Table :ref:`tab-existing-wifi-modes`) for 802.11n-based scenarios:

.. _tab-existing-wifi-modes:

.. table:: Existing Wi-Fi 802.11n modes

   ===========================   =====  =========== =========
   WifiMode name                 MCS    Short Guard Bandwidth   
   ===========================   =====  =========== =========
   OfdmRate6_5MbpsBW20MHz        0      No          20MHz
   OfdmRate13MbpsBW20MHz         1      No          20MHz
   OfdmRate19_5MbpsBW20MHz       2      No          20MHz
   OfdmRate26MbpsBW20MHz         3      No          20MHz
   OfdmRate39MbpsBW20MHz         4      No          20MHz
   OfdmRate52MbpsBW20MHz         5      No          20MHz
   OfdmRate58MbpsBW20MHz         6      No          20MHz
   OfdmRate65MbpsBW20MHz         7      No          20MHz
   OfdmRate7_2MbpsBW20MHz        0      Yes         20MHz
   OfdmRate14_4MbpsBW20MHz       1      Yes         20MHz
   OfdmRate21_7MbpsBW20MHz       2      Yes         20MHz
   OfdmRate28_9MbpsBW20MHz       3      Yes         20MHz
   OfdmRate43_3MbpsBW20MHz       4      Yes         20MHz
   OfdmRate57_8MbpsBW20MHz       5      Yes         20MHz
   OfdmRate65MbpsBW20MHzShGi     6      Yes         20MHz
   OfdmRate72_2bpsBW20MHz        7      Yes         20MHz
   OfdmRate13_5MbpsBW40MHz       0      No          40MHz
   OfdmRate27MbpsBW40MHz         1      No          40MHz
   OfdmRate40_5MbpsBW40MHz       2      No          40MHz
   OfdmRate54MbpsBW40MHz         3      No          40MHz
   OfdmRate81MbpsBW40MHz         4      No          40MHz
   OfdmRate108MbpsBW40MHz        5      No          40MHz
   OfdmRate121_5MbpsBW40MHz      6      No          40MHz
   OfdmRate135MbpsBW40MHz        7      No          40MHz
   OfdmRate15MbpsBW40MHz         0      Yes         40MHz
   OfdmRate30MbpsBW40MHz         1      Yes         40MHz
   OfdmRate45MbpsBW40MHz         2      Yes         40MHz
   OfdmRate60MbpsBW40MHz         3      Yes         40MHz
   OfdmRate90MbpsBW40MHz         4      Yes         40MHz
   OfdmRate120MbpsBW40MHz        5      Yes         40MHz
   OfdmRate135MbpsBW40MHzShGi    6      Yes         40MHz
   OfdmRate150MbpsBW40MHz        7      Yes         40MHz
   ===========================   =====  =========== =========

All of the above are for single-input, single-output (SISO) scenarios.
A detailed physical layer model of a MIMO system would include modeling
of the channel response matrix, Doppler effects, antenna correlation, 
delay spread, and other effects.  
For the current effort, however, we seek to capture the effects of 
MIMO in terms of an error rate change and/or a data rate change.
To support this, a new PHY configuration parameter, the number of antennas,
needs to be added.
Another application of MIMO is to increase the data rate by increasing
the number of spatial streams. Accordingly, additional |ns3| WifiMode values
need to be defined to support the multiple spatial stream rates.

We have added support for 1x2 receiver diversity and 2x2
dual spatial streams in both an AWGN channel and TGn Channel Model D.
To enable this modeling, |ns3|
WifiMode values were added for 2 spatial streams, corresponding to HT
MCS 8 - 15 (or VHT MCS 0 - 8 with 2 spatial streams), for each of the settings
in Table :ref:`tab-existing-wifi-modes` (long/short guard interval, 20/40 MHz
bandwidth). A new member 'numSpatialStreams' was added to the WifiMode class
to denote the number of spatial streams required for each MCS and data rate.
Many minor adjustments were made to the Wi-Fi module code to enable the system
to accept these WifiMode additions. PHY attributes 'Receivers' and
'Transmitters' are provided for users to set the number of RX and TX antennas
respectively for each node. Only nodes with 2 TX antennas can transmit with
dual spatial stream WifiModes, and only nodes with 2 RX antennas can
successfully receive them. When a node with 2 RX antennas receives a packet
using a single spatial stream WifiMode, the 1x2 receiver diversity gain is
automatically applied.

It can be analytically derived that in an AWGN channel with two equal
variance independent noise processes, one being added to each copy of the
same transmitted signal (one at each of the 2 RX antennas), the effective
noise variance in an equally weighted decision statistic is half of the original
noise variance. This is the case for 1x2 (1 transmitted spatial stream, 2 RX
antennas), and causes an effective receiver diversity boost of 3 dB SINR. 
This implicitly assumes two channels (one to each receiver) with gains
normalized to 1, and with MRC combining at the receiver, yielding a 3 dB
array gain.  In
the |ns3| implementation for AWGN, before looking up the error rate for an RX
frame (based on SINR), the receiver checks to see if the WifiMode being received
has one spatial stream. If it does and the receiver also has two RX antennas,
3 dB is added to the SINR prior to the lookup, yielding a lower error rate and
thus increasing successful RX probability.

In ([TGnSyncPhy]_, Figures 2-1 and 2-2), it is shown that the packet error rate
versus SNR is nearly identical between one and two spatial streams with
equivalent constellation mappings and code rates (i.e. MCS 0 versus MCS 8) in an
AWGN channel. This result is utilized in the |ns3| MIMO implementation for 2x2
(2 transmitted spatial streams, 2 RX antennas), by mapping the error rate lookup
for each dual spatial stream MCS to its SISO counterpart. The AWGN SISO error
rates (already included in |ns3|) are mapped as bit error rates. The error
rate for a bit chunk is determined based on the constellation mapping, code
rate, and number of bits in the chunk. Since dual spatial stream data rates
are approximately (and in most cases exactly) double that of their single
spatial stream counterparts, if the WifiMode of an RX bit stream has two
spatial streams, the number of bits to use in looking up the chunk error rate
is halved before the lookup is done. In doing so, the equivalent SISO error
rate is obtained, achieving the desired result for AWGN.

The MIMO error rate modeling approach taken for TGn Channel Model D is described
in section :ref:`Fading model TGn Channel Model D`.

To validate the capability of using the new MIMO WifiModes, the example
``examples/wireless/wifi-spectrum-saturation-example.cc`` was extended into
``examples/wireless/wifi-spectrum-saturation-example-mimo.cc`` to step through
the 2 spatial stream rates, providing saturated throughput results for each.

Fading model TGn Channel Model D
################################

The propagation loss models in use in these scenarios incorporate path
loss and shadowing components, but not fast fading components.  If no
fast fading models are applied,  then approximations to the performance 
impact of MIMO are straightforward to obtain and apply as offsets to the
received SNR or data rate, as described for the AWGN channel in section
:ref:`Enhancements to MIMO modeling`.

For Channel model D, it is not possible to use approaches such as SNR 
offset of the SISO results to characterize link level performance. As a 
result, we need to rely on bottom-up link simulations of the scenarios;
the results will depend on transmit-side configuration such as the 
modulation and coding, as well as receive-side demodulator/decoder used.

High fidelity MIMO modeling for fading channels requires several steps that 
are not yet implemented in ns-3:

1. Store each interfering signal with the pre-coding in some sort of collection
2. Wait for the reception at a given receiver so that the post-coding matrix is determined
3. Apply post-coding to the stored interference signals in order to obtain the appropriate contribution to interference

This would account for any directionality due to beam-forming at the transmitter and interference nulling at the receiver.

This would necessitate channel modeling, pre-coding and post-coding computation, and directional antenna models along with correlation assumptions between antennas, to be more detailed than what is presently in ns-3.  Therefore, we seek approximations for the current framework.

The existing ns-3 error rate model framework requires the method 'double GetChunkSuccessRate (WifiMode mode, double snr, uint32_t nbits)' to be used to look up RX error
rates for a given chunk of bits based on the WifiMode, SNR, and number of bits in the chunk.
The WifiMode and SNR are mapped to a bit error rate which is then used to calculate the success rate of the chunk.
In implementation, the SNR used to look up the error rates is actually the SINR (accounting for Wi-Fi and LTE interference).
As it has bit-size granularity, this error rate model is effective for looking up error rates for different chunks of the same Wi-Fi frame
that experience different levels of interference, and for variable length Wi-Fi frames in general.

To model receiver performance in Channel Model D, we utilize frame error rate results obtained from (separate) link simulators and map them to ns-3
for both SISO and MIMO. These link simulation results provide the probability of frame reception error vs. SNR.
As they are in terms of frame error rate, not bit error rate, we converted the frame error rates to
bit error rates before mapping them to the ns-3 error rate model framework described above. Approximating payload bit errors in a frame to be independent,
the frame error rate can be expressed as:

.. math:: Pr(frame\ error) = 1 - (1 - Pr(bit\ error))^{PLBPF}
   :label: ber_to_fer

where PLBPF is the number of payload bits per frame. Equation :eq:`ber_to_fer` can be rearranged to solve for bit error rate as:

.. math:: Pr(bit\ error) = 1 - (1 - Pr(frame\ error))^{1 / PLBPF}
   :label: fer_to_ber

Equation :eq:`fer_to_ber` was used to convert the frame error rates of the Channel Model D link simulation results to bit error rates.
It was observed in the calculated bit error rates that for each MCS, above a certain SNR threshold the bit error rate versus SNR roughly follows a log-linear relationship.
Consequently, a log-linear curve fit was done for SNRs above the threshold. Below the SNR threshold the bit error rate is 1.

The new ``FreqSelectiveErrorRateModel`` ns-3 class provides a piecewise function implementation for the Channel Model D bit error rate results.
When its 'GetChunkSuccessRate' method is called, it determines the bit error rate for the given MCS, antenna configuration (1x1, 1x2, or 2x2), and SNR.
Each MCS-antenna configuration pair has its own SNR threshold and log-linear bit error rate function. If the given SNR is below the SNR threshold, the
retrieved bit error rate is 1. Otherwise, the log-linear function is used to calculate the bit error rate for the given SNR.
From the retrieved bit error rate, the success rate of the chunk is
computed as:

.. math:: Pr(chunk\ success) = (1 - Pr(bit\ error))^{nbits}
   :label: ber_to_csr

where nbits is the number of bits in the chunk.

The UW link simulator was used to generate the Channel Model D frame error rate results
for both 1x1 (SISO) and 1x2 (1 spatial stream with 2 antenna receiver diversity) configurations. The mapped curve fitted bit error rates for 1x2 are displayed
in figure :ref:`fig-uw-linksim-1x2-ber`.

.. _fig-uw-linksim-1x2-ber:

.. figure:: figures/1x2_ber_curvefit.*
   :align: center

   Curve fitted bit error rates calculated from the UW link simulator 1x2 frame error rates

For 2x2 MIMO, we have leveraged the published frame error rate results from [TGnSyncPhy]_ Figure 10, as the UW link simulator does not have the capability to generate 2x2 results.
The mapped curve fitted 2x2 bit error rates are shown in figure :ref:`fig-tgn-linksim-ber`.

.. _fig-tgn-linksim-ber:

.. figure:: figures/2x2_ber_curvefit.*
   :align: center

   Curve fitted bit error rates calculated from the 2x2 frame error rates retrieved from [TGnSyncPhy]_

HT-aware Wi-Fi rate controls
============================

One limitation in the current |ns3| is the inability for adaptive rate
controls to select high-throughput (HT) rates.  This is an implementation
artifact in which the current operational rate set is stored in two
lists internally (one for basic modes, and one for HT-modes), but the
rate controls predate the inclusion of 802.11n support and therefore
are only aware of basic modes.  The rate controls at 5GHz are not even
aware of the full 802.11a operational rate set; only the 6Mbps, 12Mbps,
and 24Mbps rates are discoverable by rate control algorithms such
as Minstrel.  The |ns3| ConstantRateWifiManager class can be set to
any available mode, but this rate control is not adaptive with SINR.

This project has extended the |ns3| IdealWifiManager to become aware of the
HT-modes, and has configured the scenarios to use this rate control.
IdealWifiManager is an idealized (non-realizable) rate control that
has side information available about the remote station's last signal
to noise ratio level for a successful reception.  It then uses this
side information to pick the next rate for transmission.

The modified IdealWifiManager works as follows.  If the local Phy has
two antennas and the two spatial stream modes are available, a
MIMO 2 spatial stream mode will be selected (best available mode that
is supported by the currently observed SNR).  These modes are found from 
the Phy object which stores a list of the MCS modes that it supports.  
If even the lowest 2 spatial stream rate is not supported, the rate control 
will fall back to a single stream at the lowest 802.11n rate (MCS 0 SISO).

The rates are chosen by comparing the last reported SNR from the 
receiver (which is being conveyed out-of-band as this is an 'ideal' 
rate control variant that operates on side information).  The rate 
control has a target BER which defaults to 1e-6.  The physical layer and 
ns-3 error rate models support a method called 'CalculateSnr()' that allows 
a client to provide a WiFi mode and a target BER, and the Phy will return 
the lowest SNR for that mode (by consulting the error rate table and 
performing a binary search) that achieves the target BER for that mode. 

Non-synchronized LTE interference model enhancement
===================================================

The pre-existing interference model provided by the ns-3 LTE module
was relying on a simplifying assumption that all interfering signals
were LTE signals and were synchronized at the LTE subframe level. This
does not allow to simulate interference from non-LTE signals, as is
required by the simulation of the coexistence scenarios which are the
primary target of the `laa-wifi-coexistence` module.

In order to overcome this limitation, the LTE model code has been
enhanced to handle interference by signals of any type (LTE or not)
modeled with the Spectrum framework and having with arbitrary timing.

The modeling approach consists of evaluating the reception of LTE
signals by chunks, where each chunk is identified by a constant power
spectral density of interference (i.e., no new signal starting nor
previous signal ending in this period). The model is implemented using
the architecture depicted in the figures
:ref:`fig-lte-dl-interference-architecture` and 
:ref:`fig-lte-ul-interference-architecture`, which refer to the DL and
 UL, respectively. 


.. _fig-lte-dl-interference-architecture:

.. figure:: figures/lte-dl-interference-architecture.*
   :align: center

   Architecture of the LTE DL interference model


.. _fig-lte-ul-interference-architecture:

.. figure:: figures/lte-ul-interference-architecture.*
   :align: center

   Architecture of the LTE UL interference model


Different approaches are used to aggregate chunks depending on the
metric being considered and the purpose for which it is used:

 * For error modeling purposes, SINR chunks are aggregated first in
   the frequency domain using the MIESM model, thus obtaining a MI value
   per SINR chunk; then, different chunks are aggregated in the time
   domain, weighting the MI of each chunk by the duration of the
   chunk, to determine the overall MI of the TB being
   transmitted. This overall MI is then used for looking up the error
   rate according to the link-to-system mapping model. The existing
   ns-3 LTE MIESM-based error model is used for this purpose (see the
   legacy ns-3 LTE module documentation for details).

 * For modeling reference signals, aggregation in the time domain is
   not needed, as only RX power of the intended signal is evaluated
   (not interference).

 * For interference power calculations aimed at CQI feedback and
   interference reporting, time domain linear averaging of
   interference values is used;

Test coverage for this functionality is provided by the test suites
described in the sections :ref:`LTE unlicensed interference test` and
:ref:`LTE PHY error model test enhancements`.

Duty-cycled LTE transmission model
==================================

We extended the LTE model to provide support for duty-cycled
transmission as a means for LTE to allow coexistence with Wi-Fi. The
duty cycled LTE transmission model follows the approach proposed in
[Nokia2013coexistence]_ which consisting on using the LTE Almost Blank
Subframe (ABS) functionality to disable LTE transmissions in certain
subframes. 

The ABS functionality is implemented by specifying an ABS pattern of
duration 40 ms. The configuration is implemented with a bitmask of 40
values (one per LTE TTI), thus conforming to the format specified by
[TS36423]_ section 9.2.54 "ABS Information". Note that the ns-3 LTE
model does not include a detailed representation for the PBCH and RS
signals; because of this, the implemented ABS model actually behaves like
a completely blank (rather than almost blank) subframe model, i.e., in
a blank subframe no interfering signal is generated. The ABS model
involves interaction with the LTE PHY layer model to inhibit PDCCH and PDSCH
transmission, as well as with the LTE MAC layer model to inhibit the
LTE MAC scheduling of the blank subframes. 

The correct functionality of the ns-3 LTE ABS model is validated by a
dedicated test suite, which is described in Section 
:ref:`LTE ABS interference test`.

Propagation modeling
====================

This section contains a summary of the channels models that will be implemented to
evaluate the WiFi and LTE interference scenarios corresponding to the scenarios described in [TR36889]_, in the context of Task 1.7.

Indoor scenario
###############

The indoor scenario ([TR36889]_, section A.1.1) is based on two operators deploying 4 small
cells each, in one floor of a building, operating in unlicensed band. The evaluation will consider
1) both operators deploying WiFi APs, 2) both operators deploying LTE small cells, 3) one
operator deploying WiFi and the other LTE.

The reference model in [TR36889]_ for channel modeling in the indoor scenario is the ITU InH, defined in [TR36814]_. On the other hand, [80211ax]_, suggests a different propagation model, which is the one selected for implementation for the three links of interest,
(i.e. small cell to small cell/AP to AP, small cell (AP) to UE, UE to UE).
This model is summarized in the following table, where d = max(3D_distance [m],1), fc is the frequency of operation in [GHz], and W is the wall penetration loss.

+--------------------------------------------------------------------------------------+------------------------------------+
|Path Loss                                                                             |Shadow fading                       |
+======================================================================================+====================================+
|                                                                                      | Lognormal 5dB, i.i.d.              |
|                                                                                      |                                    |
| PL(d) = 40.05 + 20·log10(fc/2.4) + 20·log10(min(d,10)) + (d>10)·35·log10(d/10) + 7·W |                                    |
+--------------------------------------------------------------------------------------+------------------------------------+

Outdoor scenario
################

The outdoor scenario ([TR36889]_, section A.1.2) is based on a macro layer of 19 or 7 macro
sites operating in licensed band, and on two clusters of four small cells, managed by two operators, distributed uniformly random. Since the LTE ns3 model does not implement
currently Carrier Aggregation functionalities, the operation in the licensed band will not be
simulated. Consequently the macro cell to UE propagation
model (ITU Uma) will not be implemented in this phase of the project. The evaluation will consider 1) both operators deploying WiFi APs, 2) both operators deploying
LTE small cells, 3) one operator deploying WiFi and the other LTE.

The reference model for channel modeling in the outdoor scenario is the ITU UMi, as
suggested in [TR36889]_. This is defined in [TR36814]_. The same model is also considered in
[80211ax]_ for the outdoor scenario. We need to model the small cell-to-UE links, and the
small cell-to-small cell and UE-to-UE links. For the UE-to-UE link, [TR36889] refers to
[TR36843]_, on the study of D2D channel models. This model again refers to the ITU UMi model. We propose then to
consider the ITU UMi model for all the links.

+--------+--------------------------------------------------------------------------------------+-----------------------+----------------------------------------+
|Scenario|Path Loss                                                                             |Shadow fading          |Application range                       |
+========+======================================================================================+=======================+========================================+
|        | PL(d) = 22·log10(d) + 28 + 20·log10(fc)                                              | Lognormal 3dB, i.i.d. |10 m < d < d'BP                         |
| LOS    +--------------------------------------------------------------------------------------+-----------------------+----------------------------------------+
|	 | PL(d) = 40·log10(d) + 7.8 - 18·log(hBS)+18·log(hUE)+2·log(fc)                        | Lognormal 3dB, i.i.d. |d'BP < d < 5000, hBS=10 m, hUE=1.5 m    |
+--------+--------------------------------------------------------------------------------------+-----------------------+----------------------------------------+
| NLOS   | PL(d) = 36.7·log10(d) + 22.7 + 26·log10(fc)                                          |Lognormal 4dB, i.i.d.  |10 m < d < 2000 m, hBS=10 m, hUE=1-2.5 m|
+--------+--------------------------------------------------------------------------------------+-----------------------+----------------------------------------+

The breakpoint distance d'BP = 4·hBS·hUT·f/c, where f is the frequency of operation in Hz, c=3.0·10E8 m/s is the velocity in free space, and h'BS and h'UE are the effective antenna heights at the BS and the UE, respectively. The effective antenna heights h'BS and h'UE are computed as follows: h'BS=hBS-1.0 m, h'UE=hUE-1.0 m, where hBS and hUE are the actual antenna heights, and the effective environment height in urban environments is assumed to be equal to 1.0 m.

3) Line of Sight (LOS) Probability
A LOS/NLOS probability is included in the ITU UMi model for outdoor users only. In particular, the LOS probability PLOS = min (18/d,1)·(1-exp(-d/36))+exp(-d/36).

Scenario design
===============

There are three basic scenarios used as part of this study.  The scenarios
are motivated by, and the design follows, the scenarios described in
Appendix A of [TR36889]_.  There are only two scenarios described in
[TR36889]_ (indoor and outdoor), but we add a third simple scenario that
is used for debugging and for experimenting on a small scale.

.. _sec-simple-scenario:

Simple scenario
###############

Figure :ref:`fig-simple-scenario-layout` provides an overview of the 
simple scenario designed as a simplified version of the indoor scenario
to be described next.

.. _fig-simple-scenario-layout:

.. figure:: figures/simple-scenario-layout.*
   :align: center

   Layout of nodes for the simple scenario

The basic idea is to provide:

* the ability to toggle between WiFi and LTE in the two networks

* the ability to vary the distance 'd1' between the BS and UE, to experiment
  with different signal strength effects

* the ability to vary the distance 'd2' between the two networks, to 
  experiment with different levels of interference isolation between
  the two networks.

In addition to the depicted nodes, there is a 'backhaul' node that serves
as traffic client and that is connected by point-to-point links to both
base stations.  This node originates all traffic on the downlinks towards
the UEs.

Several other arguments can be provided to the scenario program to control
its behavior, including whether to use UDP or TCP, and others.

Indoor scenario
###############

The indoor scenario follows Appendix A.1.1 of [TR36889]_ as described herein.
We describe the two operators as "operator A" and "operator B".

Figure :ref:`fig-indoor-scenario-layout` provides an overview of the 
node layout corresponding to TR36.889.

.. _fig-indoor-scenario-layout:

.. figure:: figures/indoor-scenario-layout.*
   :align: center

   Layout of nodes for the indoor scenario

Two operators deploy four small cells in a building with no walls and
dimensions as shown.  The four base stations for each operator are equally
spaced, but the offset on the X axis between base stations for operators
A and B is controlled by a global value of the program called  "bsDistance".
The remaining UEs are randomly distributed in the rectangular region.

The indoor scenario also has these design aspects:

* **System bandwidth per carrier:** Is 20Mhz single channel centered at 5.180 GHz'

* **Number of carriers:** There is some ambiguity as to how to interpret
  the value of 4 carriers specified in [TR36889]_.  For now, the scenario
  supports 1 carrier (co-channel only).

* **Total BS TX power:**  18 dBm 

* **Total UE TX power:**  18 dBm

* **Distance-dependent path loss:** ITU InH

* **Penetration:** Do not know how to interpret the 0 dB requirement.

* **Shadowing:** ITU InH

* **Antenna pattern:** 2D omni

* **Antenna height:** Not modeled yet although should be possible (6m)

* **UE antenna height:** Not modeled yet although should be possible (1.5m)

* **antenna gain and loss:**  5dBi 

* **fast fading:** The fast fading model is not incorporated in the
  ns-3 propagation loss models in use in the indoor and outdoor
  scenarios. The Wi-Fi error rate model has been
  generated considering channel model D. The LTE error model has been
  generated considering an AWGN model [MezMio12]_. 

* **Number of UEs:** 20 per operator; 40 total.

* **UE dropping per network:** [TR36889]_ specifies that all UEs
  should be randomly dropped and be within coverage of the small cell
  in the unlicensed band. After comparing the path loss model
  characteristics with the topology specification of this scenario, we
  concluded that all UEs dropped within the indoor topology will
  satisfy the coverage requirement without the need for any further
  operation.

* **Minimum distance:** Not sure how to interpret the 3m requirement

* **Traffic model:** FTP Model 1 as in TR36.814; overall offered load is
  the same in both networks

* **UE receiver:** Not sure how to interpret the MMSE-IRC requirement

* **UE noise figure:**  9 dB 

* **UE speed**  Interpreted this to mean a fading parameter

* **Cell selection criteria:**  For Wi-Fi, strongest RSS of WiFi
  APs. For LTE, the initial cell selection procedure is evaluated
  using strongest RSRP, and cell reselection is not evaluated

* **UE bandwidth:**  For Wi-Fi, 20 MHz.  For LTE, 20MHz in the
  unlicensed band are used, with RRC signaling being transmitted
  out-of-band, in order to simulate a Carrier Aggregation setup with
  data plane transmissions occurring entirely in the unlicensed band,
  in order to have a fair comparison with Wi-Fi.
 
* **Network synchronization:** all LTE base stations are synchronized
  among themselves. There is no synchronization between LTE base
  stations and Wi-Fi base stations.

* **DL/UL traffic ratio:** 100% on DL 


Outdoor scenario
################

The outdoor scenario follows Appendix A.1.2 of [TR36889]_ as described herein.
We describe the two operators as "operator A" and "operator B".

The scenario consists of macro cells and licensed and unlicensed small cells,
with clusters uniformly random within the macro geographical area, and
4 small cells per operator.

Figure :ref:`fig-outdoor-scenario-rem` provides a radio environment map of
the node layout.

.. _fig-laa-wifi-outdoor-topology_-250_1335_-360_1220_200x200:

.. figure:: figures/laa-wifi-outdoor-topology_-250_1335_-360_1220_200x200.*
   :align: center

   Topology of the outdoor scenario

The outdoor scenario also has these design aspects:

* **Outdoor layout** hexagonal macro cell layout with 7 macro sites
  and 3 cells per site; within the hexagonal area corresponding to
  each cell, 1 cluster is randomly placed following the specifications
  of [TR36889]_, and 4 small cells per operator are randomly deployed
  in the cluster following the specifications of [TR36889]_. Macro
  LTE base stations in the licensed bands are not simulated; the
  position of macro cell sites is considered only for determining the
  position of the small cell clusters.

* **System bandwidth per carrier** 20 MHz are considered for both LTE
  and Wi-Fi.

* **Carrier frequency**  5.180 GHz

* **Number of carriers** There is some ambiguity as to how to interpret
  the value of 4 carriers specified in [TR36889]_.  For now, the scenario
  supports 1 carrier (co-channel only).

* **Total BS TX power:**  18 dBm 

* **Total UE TX power:**  18 dBm

* **Distance-dependent path loss:** ITU UMi

* **Penetration:** 0 dB as specified by [TR36889]_ for outdoor UEs

* **Shadowing:** ITU UMi

* **Cell selection criteria** For LAA UEs, cell selection is based on RSRP in the unlicensed band. For WiFi STAs, cell selection is based on RSS (Received signal power strength) of WiFi APs. RSS threshold is -82 dBm.

* **Antenna pattern:** 2D omni

* **Antenna height:** 10m antenna height is considered in the path
  loss model calculation. Aside from this, base stations are placed at
  0m height, hence all distance calculations (including those used to
  determine the path loss component) refer to 2D distance.

* **UE antenna height:** 1.5m antenna height is considered in the path
  loss model calculation.  Aside from this, UE are placed at
  0m height, hence all distance calculations (including those used to
  determine the path loss component) refer to 2D distance.

* **antenna gain and loss:**  5dBi for base stations; 0dBi for UEs.

* **fast fading:** The fast fading model is not incorporated in the
  ns-3 propagation loss models in use in the indoor and outdoor
  scenarios. The Wi-Fi error rate model has been
  generated considering channel model D. The LTE error model has been
  generated considering an AWGN model [MezMio12]_. 

* **Number of clusters per macro cell geographical area:** 1

* **Number of UEs** There is some ambiguity as to how to interpret the
  value of 20 UEs per unlicensed band carrier per operator specified in
  [TR36889]_. Currently we are considering 20 UEs per cluster.

* **UE dropping for each network:** [TR36889]_ specifies that all UEs
  should be randomly dropped and be within coverage of the small cell
  in the unlicensed band. After comparing the path loss model
  characteristics with the topology specification of this scenario, we
  concluded that all UEs dropped within the specified radius from the
  cluster center will satisfy the coverage requirement without the need for any further
  operation.

* **Radius for small cell dropping in a cluster** 50 m

* **Radius for UE dropping in a cluster** 70 m

* **Minimum distance (2D distance):** 

   * Small cell-small cell: 20m, enforced by randomly dropping base
     stations sequentially, and repeating each dropping until the min
     distance requirement is satisfied for all the previously dropped
     small cells within the same cluster.
     
   * inter-operator small cell-small cell: 10m, enforced by dropping
     the base stations of one operator first, and then iteratively
     dropping the base stations of the other operator one by one,
     repeating each dropping  until the min distance requirement is
     satisfied for all the base stations of the first operator within
     the same cluster (additionally to other constraints).

   * small cell-UE and UE-UE: 3m, enforced by randomly dropping
     UEs sequentially, and repeating each dropping until the min
     distance requirement is satisfied for all the previously dropped
     UEs and base stations.
     
   * macro-small cell cluster center: 105m, enforced by repeating the
     random placement of the cluster center until the constraint is
     satisfied.

   * macro-UE: 35m; enforced by repeating the dropping of each UE if
     the constraint is not satisfied.

   * cluster-center-cluster: 100m, enforced by reducing the radius of
     the macro cell geographic area by 50m, so that the single cluster
     to be dropped within the cluster region will meet the constraint
     with respect to each cluster to be deployed in the other macro
     cell geographic areas.

* **UE receiver:** Not sure how to interpret the MMSE-IRC requirement

* **UE noise figure:**  9 dB 

* **UE speed:**  Interpreted this to mean a fading parameter

* **Cell selection criteria:**  For Wi-Fi, strongest RSS of WiFi
  APs. For LTE, the initial cell selection procedure is evaluated
  using strongest RSRP, and cell reselection is not evaluated

* **UE bandwidth:**  For Wi-Fi, 20 MHz.  For LTE, 20MHz in the
  unlicensed band are used, with RRC signaling being transmitted
  out-of-band, in order to simulate a Carrier Aggregation setup with
  data plane transmissions occurring entirely in the unlicensed band,
  in order to have a fair comparison with Wi-Fi.
 
* **Network synchronization:** all LTE base stations are synchronized
  among themselves. There is no synchronization between LTE base
  stations and Wi-Fi base stations.

 * **Backhaul assumptions:** non-ideal backhaul is assumed for all
   base stations.

* **DL/UL traffic ratio:** 100% on DL.




Traffic models
##############

TR36.889 calls for several traffic models.

1.  FTP Model 3, which is a derivative of FTP model 2 defined in TR 36.814.
    There are some open issues with respect to how to interpret and
    construct this model.

2.  FTP Model 1 defined in TR 36.814.  This model simulates file transfers
    arriving according to a Poisson process with arrival rate lambda.  The
    lambdas recommended range from 0.5 to 2.5.  The file size is 2 Mbytes with
    0.5 Mbytes optional in TR 36.814, but TR 36.889 requests the 0.5 Mbytes
    size.

3.  (Optional) Mixed traffic model including VoIP flows in the Wi-Fi network.
    The VoIP traffic model is based on G.729A with a data rate of 24 kbps,
    a packet interarrival time of 20 ms, and a packet size of 60 bytes.

In addition, control plane traffic is not modeled, and the document specifies
percentage splits between downlink and uplink traffic (such as 50%/50% or
80%/20%.

The overall offered load is to be the same for both coexisting networks.

In |ns3|, to date we have implemented the FTP Model 1 for this project, 
and evaluate it on the downlink only according to different lambda values.

In addition, we provide a saturating UDP flow option.  When UDP is selected,
the scenario is configured to generate about 80 Mb/s of traffic on the
downlink in each operator network.  When there is a single client
(e.g., in the simple scenario each operator has a single client), 
there is only a single flow.  When there are
multiple clients and cells, the flow is distributed across the UEs such
that each cell gets approximately 80 Mb/s offered load. The value has
been chosen to saturate the PHY channel and hence result in
full-buffer simulation even in the case that the UEs are close to the
base stations. 


Performance metrics
###################

Performance metrics are described in TR 36.889 and have been expanded
upon in the latest version of the document.  The main performance
metrics are 'user perceived throughput' and 'latency', plotted as
CDFs, for a given scenario.

In |ns3|, we are calculating these by using the built-in FlowMonitor
tool that tracks per-flow statistics including throughput and latency,
and we then post-process these results to obtain CDFs.

A new metric 'Average Buffer Occupancy (BO)' has recently been added
to TR 36.889; we do not have any support for that yet in |ns3|.

.. only:: html
References
==========

.. [TR36889] 3GPP TR 36.889 "Study on Licensed-Assisted Access to Unlicensed Spectrum", (Release 12) TR 36.889v0.1.1 (2014-11), 3rd Generation Partnership Project, 2014.
.. [TR36814] 3GPP TR 36.814 "Further advancements for E-UTRA physical layer aspects", (Release 9), 3rd Generation Partnership Project, 2014.
.. [TR36843] 3GPP TR 36.843 "Study on LTE Device to Device Proximity Services; Radio Aspects", (Release 12), 3rd Generation Partnership Project, 2014.
.. [TS36423] 3GPP TS 36.423 "E-UTRAN X2 Application Protocol (X2AP)"
.. [80211ax] IEEE 802.11 "TGax Simulation Scenarios", doc.: IEEE 802.11-14/0980r9, July 2014.
.. [PhySimWifi] S. Papanastasiou, J. Mittag, E. Ström, H. Hartenstein, "Bridging the Gap between Physical Layer Emulation and Network Simulation," Proceedings of IEEE Wireless Communications and Networking Conference, Sydney, Australia, April 2010.  Available online at https://dsn.tm.kit.edu/english/ns3-physim.php.
.. [BalMio09] N. Baldo and M. Miozzo, "Spectrum-aware channel and PHY layer modeling for ns3," Proceedings of the Fourth International ICST Conference on Performance Evaluation Methodologies and Tools (VALUETOOLS 2009), Pisa Italy, 2009.
.. [Nokia2013coexistence] Erika Almeida et al., "Enabling LTE/WiFi coexistence by LTE blank subframe allocation", in Proc. of IEEE ICC 2013.
.. [TGnChannelModels] V. Erceg, L. Schumacher, P. Kyritsi et al., “Tgn channel models,” IEEE, Tech. Rep. 802.11-03/940r4, May 2004.
.. [DelayedAutocorrSync] K. Wang, M. Faulkner, J. Singh, and I. Tolochko, “Timing synchronization for 802.11a wlans under multipath channels,” Centre for Telecommunications and Microelectronics, Victoria University. [Online]. Available: http://cegt201.bradley.edu/projects/proj2006/sradio/archives/Timing_sync_802_wlan_multipath.pdf
.. [TGnSyncPhy] S. A. Mujtaba, "TGnSync proposal PHY results," IEEE, Tech. Rep., May 2005.
.. [MezMio12] Marco Mezzavilla, Marco Miozzo, Michele Rossi, Nicola Baldo, Michele Zorzi, "A Lightweight and Accurate Link Abstraction Model for the Simulation of LTE Networks in ns-3", in Proceedings of ACM MSWiM 2012.
.. [Catreux2003] S. Catreux et al., "Some Results and Insights on the Performance Gains of MIMO Systems," IEEE JSAC 2003.



Usage
*****

This section will provide overviews of some of the example programs, and
describe how parameters can be controlled and changed, and what output
is produced.

Examples
========

Several small example programs are provided to highlight the operation.


wifi-spectrum-saturation-example.cc
###################################
The program ``examples/wireless/wifi-spectrum-saturation-example.cc``
allows user to select either the `SpectrumWifiPhy` or `YansWifiPhy` for 
saturation tests.  The wifiType can be toggled by the argument 
``'--wifiType=ns3::YansWifiPhy'`` or ``--wifiType=ns3::SpectrumWifiPhy'``

There isn't any difference in the output, which is to be expected because 
this test is more of a test of the DCF than the physical layer.

By default, the program will use the `SpectrumWifiPhy` and will run
for 10 seconds of saturating UDP data, with A-MPDU and Block Ack enabled.
It produces this output for the main 802.11n rates (with short and long guard
intervals):

::
  
  ./waf --run wifi-spectrum-saturation-example
    
  WifiType=ns3::SpectrumWifiPhy
  DataRate	Throughput
  6.5		6.02813 Mbit/s
  13		12.0539 Mbit/s
  19.5		18.0832 Mbit/s
  26		24.1102 Mbit/s
  39		36.2112 Mbit/s
  52		48.2816 Mbit/s
  58.5		54.179 Mbit/s
  65		59.9999 Mbit/s
  ...
  150           134.206 Mbit/s

The above output shows the first 8 (of 32) modes, and last mode, that will be
output from the program.  The first 8 modes correspond
to short guard interval disabled and channel bonding disabled.  The
subsequent 24 modes run by this program are variations with short guard
interval enabled (cases 9-16), and then with channel bonding enabled and
short guard first disabled then enabled (cases 17-32). 

When run with the legacy YansWifiPhy:

::

  ./waf --run "wifi-spectrum-saturation-example --wifiType=ns3::YansWifiPhy"
  

the output should be the same; this is to be expected since YansWifiPhy 
and SpectrumWifiPhy share the same error rate model in this case.

wifi-spectrum-saturation-example-mimo.cc
########################################
The program ``examples/wireless/wifi-spectrum-saturation-example-mimo.cc``
expands on the previous program to include also the MIMO rates.  Rather than
32 output values, it generates 64 (because a case of MIMO with 2x2 two spatial
streams is run for each single stream case).  It generates the following 
output:

::
  
  ./waf --run wifi-spectrum-saturation-example-mimo
    
  WifiType=ns3::SpectrumWifiPhy
  DataRate	RX/TX	Throughput
  6.5		1x1	6.02813 Mbit/s
  13		1x1	12.0539 Mbit/s
  19.5		1x1	18.0832 Mbit/s
  ...
  270		2x2	229.876 Mbit/s
  300		2x2	252.388 Mbit/s

wifi-spectrum-per-example.cc 
############################
The program ``examples/wireless/wifi-spectrum-per-example.cc`` allows users 
to select either `SpectrumWifiPhy` or `YansWifiPhy`, as above, and select 
the distance between the nodes, and to log the reception statistics and 
received SNR (as observed by the MonitorRx trace source), using a
Friis loss model.  By default, it steps through the MCS values for a 
50m distance, producing output such as:

::

   ./waf --run "wifi-spectrum-per-example"

   wifiType: ns3::SpectrumWifiPhy distance: 50m; sent: 1000
   MCS     DataRate        Throughput      Received  Signal (dBm) Noise (dBm) SNR (dB)
   0       6.5             0.7776 Mbit/s   1000       -77.6633    -100.966    23.3027
   1       13              0.7776 Mbit/s   1000       -77.6633    -100.966    23.3027
   ...

and

::

   ./waf --run "wifi-spectrum-per-example --wifiType=ns3::YansWifiPhy"

will yield the same result.

By increasing the distance, lowering of SNR and frame losses can be 
observed.  Distance is lowered by using the ``--distance`` argument.

The use of `SpectrumWifiPhy` or `YansWifiPhy` doesn't make any difference; 
this helps to validate the new model because in the absence of non-Wi-Fi 
signals, the code is behaving the same (because it is using the same 
underlying `InterferenceHelper` and same `ErrorRateModel`).

The full set of program arguments is found with:

::
  
  ./waf --run "wifi-spectrum-per-example --help"

  ns3-dev-wifi-spectrum-per-example-debug [Program Arguments] [General Arguments]

  Program Arguments:
    --simulationTime:  Simulation time in seconds [10]
    --udp:             UDP if set to 1, TCP otherwise [true]
    --distance:        meters separation between nodes [50]
    --mcs:             restrict MCS to single value between 0 and 31 [256]
    --wifiType:        select ns3::SpectrumWifiPhy or ns3::YansWifiPhy [ns3::SpectrumWifiPhy]
    --errorModelType:  select ns3::NistErrorRateModel or ns3::YansErrorRateModel or ns3::FreqSelectiveErrorRateModel [ns3::NistErrorRateModel]
    --enablePcap:      enable pcap output [false]

  General Arguments:
    --PrintGlobals:              Print the list of globals.
    --PrintGroups:               Print the list of groups.
    --PrintGroup=[group]:        Print all TypeIds of group.
    --PrintTypeIds:              Print all TypeIds.
    --PrintAttributes=[typeid]:  Print all attributes of typeid.
    --PrintHelp:                 Print this help message.

Another possible option is to change the error model type, to experiment
with the new frequency-selective error rate model (the |ns3| default
of NistErrorRateModel is used as the program default).

wifi-spectrum-per-interference.cc 
#################################
The program ``examples/wireless/wifi-spectrum-per-interference.cc`` is based 
on the previous packet error rate example, but copies over the 
WaveformGenerator from the unlicensed LTE interferer test, to allow 
users to inject the non-Wi-Fi signal (using the ``--waveformPower`` argument) 
from the command line.  The default is no interference, but by adding small 
amounts of power (e.g. ``--waveformPower=0.001``), one will start to observe 
SNR degradation and frame loss.

Some sample output with default arguments (no interference) is:

::

  ./waf --run "wifi-spectrum-per-interference"

  wifiType: ns3::SpectrumWifiPhy distance: 50m; sent: 1000
  MCS 	DataRateThroughput	Received  Signal (dBm) Noi/Inf(dBm) SNR(dB)
  0	6.5	0.7776 Mbit/s	1000	   -62.6427    -100.966    38.3233
  1	13	0.7776 Mbit/s	1000	   -62.6427    -100.966    38.3233
  2	19.5	0.7776 Mbit/s	1000	   -62.6427    -100.966    38.3233

while a small amount of waveform power will cause frame losses to occur at
higher order modulations:

::

  ./waf --run "wifi-spectrum-per-interference --waveformPower=0.001"

  wifiType: ns3::SpectrumWifiPhy distance: 50m; sent: 1000
  MCS 	DataRateThroughput	Received  Signal (dBm) Noi/Inf(dBm) SNR(dB)
  0	6.5	0.7776 Mbit/s	1000	   -62.6427    -86.1032    23.4605
  1	13	0.7776 Mbit/s	1000	   -62.6427    -86.1032    23.4605
  2	19.5	0.7776 Mbit/s	1000	   -62.6427    -86.1032    23.4605
  3	26      0.7776 Mbit/s	1000	   -62.6427    -86.1032    23.4605
  4	39	0.7776 Mbit/s	1000	   -62.6427    -86.1032    23.4605
  5	52      0 Mbit/s        0	   -62.6427    -86.4552    23.8125
  6	58.5	0 Mbit/s	0	   -62.6427    -86.4552    23.8125


Validation
**********

In this section we first describe the calibration process followed to achieve comparable results from the Wi-Fi and LTE modules. Then we describe the test suites that are provided, along
with the code of the ``laa-wifi-coexistence`` module, in order to validate its
proper functionality and correct simulation output in some key
configurations for which the intended behavior and output can be
determined a priori.

Calibration of Wi-Fi and LTE modules
====================================
The meaningful simulation parameters in the LTE and Wi-Fi modules have been jointly configured based on the information available in [TR36889]_.
The behaviour of each module has been evaluated and analyzed in comparison to the other one, in order to get comparable results. A certain offset in performance curves is expected to exist due to the natural characteristics of the two different technologies.
In this sense, the following considerations have to be taken into account. 
1. in Wi-Fi we must first locate/synchronize to a frame. Most of the time, this only happens at few dB above the noise floor. On the other hand, LTE model includes a DL synchronization model that is based on the correct decoding of PCFICH and PDCCH. In general, this synchronization phase is able to properly work up to few dBs below zero. 
2. LTE has in general more advanced physical layer properties than Wi-Fi, so, assuming frame synchronization has been correctly obtained, an offset may also be observed due to coding performance. 
3. Differences may also exist in terms of MAC overhead in Wi-Fi and RLC overhead in LTE.

Taking into account the above mentioned considerations, in 
Figure :ref:`fig-validate_saturation_d2_1000_throughput_vs_rxpower` we have 
compared results in terms of throughput, as a function of the received power, 
for the simple scenario, with d2=1000 m, where the interference is negligible.
Results show that the overall slope behaviour of the two 
curves match, and the main discrepancies only exist in the low range of 
received power values, where the considerations detailed above have a major 
impact. The differences in absolute values of throughput obtained by the two 
models, in the high range of received power values, mainly depend on the 
different MIMO/SISO models.


.. _fig-validate_saturation_d2_1000_throughput_vs_rxpower:

.. figure:: figures/validate_saturation_d2_1000_throughput_vs_rxpower.*
   :align: center

   Throughput performances as a function of the received power, for Wi-Fi and LTE models

This figure can be reproduced by running the 
``validate-saturation-throughput.sh`` script found in 
``src/laa-wifi-coexistence/doc/validate-saturation-throughput/`` directory,
then running, as a second step, the script ``plot-saturation-throughput.sh``
in the same directory, and looking at the resulting figures in the ``images`` 
directory.

For Wi-Fi, readers will note that there are discontinuities in the range
of 30-50 Mb/s where the performance briefly flattens with respect to received
power.  This is due to the discrete jumps between modulation and coding modes
and the difference in performance between such modes.

The Wi-Fi performance curves were checked against the underlying error models.
In this example, the receiver noise floor is around -92 dBm, based on
a thermal noise of -174 dBm/Hz, a 20 MHz bandwidth, and a 9 dB noise
figure (at the UE on the downlink).  Using the default error rate model
for |ns3| Wi-Fi, the  most robust OFDM mode of 6 Mb/s is able to deliver a 
packets when the SNR exceeds roughly 1 dB.  It can be seen on this figure
that the last non-zero data point is at approximately a received signal
power of -91 dBm. 

Propagation Models validation
=============================

The propagation models have been validated visually by running an
example program, `main-propagation-loss` which tests the return value
of propagation models of different kinds over a pre-defined range of
distance values. For each distance, the program generates
100000 receiving nodes placed at that chosen distance from a
transmitter node emitting a signal at a transmission power of
:math:`18` dBm, and evaluates the propagation models to calculate the
power received by the receiving node. The program then partitions the
received power values for each distance into bins of range 1 dBm, and calculates the
empirical mass probability function of the bins. 

Figures :ref:`ieee80211ax-indoor-propagation-loss` and
:ref:`fig-itu-umi-propagation-loss` report the obtained results for
the Ieee80211axIndoorPropagationLossModel and the
ItuUmiPropagationLossModel respectively. 
In the case of ItuUmiPropagationLossModel we note that the
distribution for a given distance is bimodal, with the two modes
corresponding to the LOS and to the NLOS variants of the model,
respectively; we also note that the LOS model is more frequently used
at short distances, whereas the NLOS model occurs more frequently at
large distances, consistently with the ITU UMi model specification.
In both figures we note the gaussian distribution of received power
values (for each mode) which is consistent with the modeling assumption of log normal shadowing.


.. _fig-ieee80211ax-indoor-propagation-loss:

.. figure:: figures/ieee80211ax-indoor-propagation-loss.*
   :align: center

   Probabilistic distribution of the received signal strength obtained with the implemented indoor propagation model



.. _fig-itu-umi-propagation-loss:

.. figure:: figures/itu-umi-propagation-loss.*
   :align: center

   Probabilistic distribution of the received signal strength obtained with the implemented indoor propagation model


Further outdoor propagation model validation
============================================

Further validation has been carried out to verify the correctness of
the propagation model in the outdoor scenario. The validation approach
is to generate a visual representation of the propagation behavior
resulting from simulation output, and manually checking whether the
observed behavior corresponds to the expected one. The findings are
reported in the following. 

Figure :ref:`fig-outdoor-scenario-rem` provides a radio environment
map (REM) of the outdoor scenario; the REM is generated running the
script
`src/laa-wifi-coexistence/campaigns/laa-wifi-outdoor/plot_rem_outdoor.sh`. From
the figure, we note that the propagation loss has a significant random
component, that the dependency of SINR on distance looks rather weak,
and that the scenario appears mostly dominated by interference. 
 - 

.. _fig-laa-wifi-outdoor-rem_-250_1335_-360_1220_200x200:

.. figure:: figures/laa-wifi-outdoor-rem_-250_1335_-360_1220_200x200.*
   :align: center

   Radio environment map for the outdoor scenario

To better understand the plot, we first recall that the REM is
generated with a separate evaluation of the propagation model for each
pixel, and that the ITU-UMi propagation loss model includes actually
two models, a LOS model and a NLOS model, which are chosen using
i.i.d. probability that varies with distance, and that on top of that
both the LOS and the NLOS model add an i.i.d. log-normal
shadowing. This means that, when evaluating the signal from a set of
BSs to a particular point, some of these BSs will have a LOS link to
that point, and others will have a NLOS link. 

Figures :ref:`fig-figures/laa-wifi-itu-umi-rx-power` and
:ref:`fig-figures/laa-wifi-itu-umi-plos` highlight several important
facts: 

 1. a NLOS link is definitely out of range beyond 300m
 2. a LOS link is still possibly within range at 2000m, if
    interference is negligible
 3. beyond 100m distance from any BS, the difference in rx power between LOS and NLOS is so
    high that if there is at least one BS with a LOS link then it will
    have a RX power much stronger than any interfering BS with NLOS
    link. Hence NLOS links become negligible.
 4. The probability of having a LOS link is 0.01 at 2000m. This means
    that at 2000m from a total of 168 BSs (which is the total BS count
    in this scenario) we will have:

    #. :math:`\sim18\%` probability of not having any LOS BS (i.e.,
       all signals will be below noise threshold)  
    #. :math:`\sim31\%` probability of having exactly 1 LOS BS (SNR
       around 10 dB, negligible interference) 
    #. :math:`\sim26\%` probability of having exactly 2 LOS BS (RX
       power will be similar, hence SINR will be around 0 dB)
    #. for :math:`N > 2` LOS, interference dominates and SINR will be
       below 0

.. _fig-figures/laa-wifi-itu-umi-rx-power:

.. figure:: figures/laa-wifi-itu-umi-rx-power.*
   :align: center

   Received power as a function of distance for the ITU-UMi model
   evaluated with the parameters used for the outdoor scenario 


.. _fig-figures/laa-wifi-itu-umi-plos:

.. figure:: figures/laa-wifi-itu-umi-plos.*
   :align: center

   Probability of LOS as a function of distance for the ITU-UMi model
   evaluated with the parameters used for the outdoor scenario 



To conclude, the performance at any point far away from the
topology is highly random and dominated by the number of LOS
links. Only when the distance from the whole topology is increasing
significantly the propagation converges consistently to an
out-of-range scenario. The extended REM reported in figure
:ref:`fig-laa-wifi-outdoor-rem_-2250_1335_-2360_1220_200x200` confirms
this interpretation.


.. _fig-laa-wifi-outdoor-rem_-2250_1335_-2360_1220_200x200:

.. figure:: figures/laa-wifi-outdoor-rem_-2250_1335_-2360_1220_200x200.*
   :align: center

   Radio environment map for the outdoor scenario highlighting the
   propagation model behavior at large distances.





SpectrumWifiPhy verification
============================

The SpectrumWifiPhy implementation has been verified to produce equivalent
results to the legacy YansWifiPhy by using the saturation and packet
error rate programs described above in Sections 2.1.1 and 2.1.2
(respectively) and toggling the implementation between the two
physical layers.

The SpectrumWifiPhy implementation has been lightly verified to
produce reasonable results by running the example interference program
described in Section 2.1.3 and by running the simple scenario program
described in Section 2.1.4 with the LTE configuration, and observing
the behavior as distances between nodes are altered.  There hasn't
been a more systematic verification yet that all foreign signal 
power is being appropriately accounted for.  The simplified model should
also be validated that the coarse approximation of the LTE data channel
as being modeled as noise into Wi-Fi is acceptable.


Tests
=====

The following test suites have been added.

LTE unlicensed interference test
################################

The test suite `lte-unlicensed-interference` aims at verifying the
correct behavior and simulation output of the LTE PHY and, in
particular, of its interference model, in presence of non-LTE
interferes. The topology of the test is depicted in Figure
:ref:`fig-lte-unlicensed-interference-test-scenario`, and involves an
LTE eNB, an LTE UE attached to it, and a non-LTE interferer
implemented using the WaveformGenerator class. The LTE downlink is
configured in the 5GHz bandwidth where the interference also takes
place. The waveform generator is configured to transmit
continuously at constant power. The :math:`d_1` parameter represents the distance of each UE to the eNB it
is attached to, whereas the :math:`d_2` parameter represent the
interferer distance; different test cases are generated changing the
values of the parameters.

The following conditions are verified:

 - the SINR calculated at the UE is equal to the expected one within a
   tolerance of :math:`0.01` dB

 - the MCS used for transmission by the eNB is exactly the expected
   one

The reference values are calculated using the octave script
`lte_link_budget_unlicensed_interference.m` which can be found in
`src/laa-wifi-coexistence/test/reference/`


.. _fig-lte-unlicensed-interference-test-scenario:

.. figure:: figures/lte-unlicensed-interference-test-scenario.*
   :align: center

   Topology of the LTE unlicensed interference test



LTE PHY error model test enhancements
#####################################

The legacy test `lte-phy-error-model` has been enhanced to cover UL
data transmissions as well, in order to provide test coverage for the
new time-and-frequency MIESM-based model. The test conditions and test
vectors have been taken from the DL tests, which are deemed
appropriate for testing the UL RX processing chain as well.



LTE ABS interference test
#########################

The test suite `lte-interference-abs` aims at verifying the
correct transmission and interference behavior of the LTE ABS
model, in an LTE-only scenario. Two LTE eNB, each with an associated
UE, are deployed in a topology
similar to the `lte-interference` test suite. 
The :math:`d_1` parameter represents the distance of each UE to the
eNB it is attached to, whereas the :math:`d_2` parameter represent the
distance between one eNB and the UE associated to the other eNB. The
two eNBs are associated with either equal or complementary (i.e.,
orthogonal) ABS patterns. Different test cases are generated changing the 
values of the :math:`d_1` and :math:`d_2` parameters as well as the
considered ABS patterns. 

Each test case passes if the following conditions are verified:

 - the SINR calculated at the UE is equal to the expected one within a
   tolerance of :math:`0.01` dB

 - the MCS used for transmission by the eNB is exactly the expected
   one

 - all transmissions occur in subframes indicated as zero (non-blank) in the
   ABS pattern.


The reference values are calculated using the octave script
`lte_link_budget_interference_abs.m` which can be found in
`src/laa-wifi-coexistence/test/reference/`. This script is similar to
the one used for the `lte-interference` test; in particular, the
reference values are exactly the same when equal ABS pattern are used,
and are instead calculated assuming no interference when orthogonal
ABS patterns are used.


Open Issues and Future Work
***************************

This section contains discussion regarding open issues with the current
set of models and scenarios, or open questions regarding interpretation
of standards documents.

HT-aware Wi-Fi rate controls
============================

A current limitation for |ns3| is that none of the rate controls available 
are aware of the HT rates.  We now have introduced some MIMO rates, but 
MIMO must be configured manually as a "constant rate" rate control; 
i.e. it is not adaptive. 

The Linux implementation of minstrel_ht would be a good adaptive rate control 
to implement, but it is beyond the scope of the current project.  However,
an open source contributor has proposed such a model as of July 2015, and
the proposal is under review by ns-3 maintainers. 

However, we believe that some adaptive rate control is needed for the 
current scenarios.  IEEE Document 11-14/0386r0, 
"Discussions on MCS selection", discusses three possible rate controls: 

1.  Goodput-maximizing MCS: based on flow’s SINR statistics, find MCS that maximizes goodput=(1-PER)*rate 

2. 10% PER MCS: find highest MCS such that PER <= 10% 

3. Genie MCS selection: select the MCS based on instantaneous RX SINR 

There is an |ns3| "Ideal" rate control that corresponds to the Genie mode 
(with MCS selection based on knowledge of most recent Rx SINR that has 
been acknowledged).  We extended the Ideal rate control for this 
project to pick from among the newly available MCS. 

Application of 1x2 Wi-Fi configurations
=======================================

TR36.889 asks for 1x2 Wi-Fi configurations to be supported in simulation 
scenarios, presumably on the uplink.  We have implemented error rate mapping
for the 1x2 case, but the error rate model (the generic API) is not 
presently aware of the antenna configuration and the rate control therefore does
not have sufficient information to know which
error rates (1x2 versus 1x1) to apply when choosing which rate to transmit.

Since we are focusing on downlink performance for this phase of the study,
the postponement of 1x2 rate control support should not affect current results.
Receivers with two antennas will still benefit from the 1x2 diversity gain, but
the rate control of the transmitting node will not be aware of this.

Wi-Fi MIMO and interference models for fading model D
=====================================================

We have implemented error rate models obtained from link
simulations to account for the link performance in the frequency
selective TGn Channel Model D. This method decouples any frequency selectivity
and fast fading from the ns-3 model---it is all incorporated into the link
simulation error rate results.
However, the applicability of this approach to fading channel models
is an open question.  Appropriate modeling of MIMO and interference will
ultimately be a trade of the fidelity offered by finer grained approaches
to channel modeling, pre-coding and post-coding modeling, interference
bookkeeping, beamforming and antenna nulling etc., with the cost (both
run time cost of the model, and the labor involved to develop such
models) of these models.

LTE MIMO TX Mode adaptation
===========================

The LTE model supports a MIMO model, based on [Catreux2003]_,
however it does not currently support any dynamically adaptive MIMO TX
mode selection. Hence, currently, either SISO or MIMO is selected
statically upon simulation configuration. The simulation campaigns
executed so far only considered SISO. To have meaningful results for
MIMO, a dynamically adaptive TX mode selection model shall probably be
developed.

Additional traffic models
=========================

The FTP Model 3 is not implemented, and the description is ambiguous
as to how to model that "packets for the same UE arrive according to a 
Poisson process".  This is an FTP process so Poisson arrivals at the packet
level are unclear.

VoIP traffic models, and distribution of traffic loads between DL and UL,
have yet to be implemented in the scenarios.

Additional performance metrics
==============================

A number of refinements to the performance metrics have yet to be added
and could be the subject of future work:

* Support for statistics on unfinished FTP file transfers as specified
  in TR 36.889

* Support for VoIP statistics including 98th percentile latency greater than 
  50 ms as specified in TR 36.889 

* Percentage of outage VoIP users
 
* Average buffer occupancy (BO)

* Ratio of mean served cell throughput and offered cell throughput independently for DL and for UL

Scenario results for Channel Model D
====================================

We have not run simulation campaigns using the new error rate models
for Channel Model D.

LTE over LTE simulations
========================

Configuration for LTE over LTE simulations is already provided for all
scenarios; however, this functionality was considered lower priority
and hence has not been thoroughly tested. More testing is needed to
consider it fully operational.

