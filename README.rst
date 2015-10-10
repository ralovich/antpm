=========
`ANT+minus <http://code.google.com/p/antpm>`_
=========

Userspace implementation of a wire protocol similar to the
ANT/ANT+/ANT-FS protocols. The goal is to be able to communicate with
the Forerunner 310XT (or any other ANT/ANT+/ANT-FS capable device)
watch in order to retrieve sports tracks.

The C++ implementation is currently available under both Linux and
win. Communication with watches other than the 310XT might (610, 910XT
and Swim are likely) work, but are untested. This project currently
does not yet support Forerunner 405/410, but work is underway for
405/410 support. Please report your experience to help improving the
software.

ANT+minus is donationware_. If you are using ANT+minus and like it, we
would be happy if you donate an amount of your choice. A suggestion is
10 to 20 Euros/USD/GBP. In this way, you encourage the development of
new versions.

.. image:: https://www.paypalobjects.com/en_US/i/btn/btn_donateCC_LG.gif
   :alt: Donate to antpm
   :target: https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=B4AWQUT3B3YYN
   :width: 147px
   :height: 47px

This project is in no way associated with Garmin.

.. _donationware: http://en.wikipedia.org/wiki/Donationware

.. image:: https://secure.travis-ci.org/ralovich/antpm.png
   :alt: Build Status
   :target: http://travis-ci.org/ralovich/antpm
   :width: 77px
   :height: 19px

.. image:: https://scan.coverity.com/projects/2691/badge.svg
   :alt: Coverity Status
   :target: https://scan.coverity.com/projects/2691
   :width: 95px
   :height: 18px
