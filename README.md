# arduino-ble-ident-n-set
Arduino BLE module identification and setup sketch. 

Automatically identifies the module type, shows main settings and allows to change them.  
Supports HM-10, CC41-A, MLT-BT05 and similar generic BLE modules. Looking to add support for other BLE modules.

Supported modules identify themselves with the following keywords via the AT-command interface: www.jnhuamao.cn, HMSoft V540, HMSoft, www.bolutek.com, Firmware V3.0.6,Bluetooth V4.0 LE, BT05, MLT-BT05-V4.0, MLT-BT05-V4.1, MLT-BT05-V4.2, MLT-BT05, JDY-09-V4.3, JDY-09, AT-09, etc.

To learn more about Arduino BLE modules and to understand the background for this sketch, please read the following blog posts:

- https://blog.yavilevich.com/2016/12/hm-10-or-cc41-a-module-automatic-arduino-ble-module-identification/ , 
- https://blog.yavilevich.com/2017/03/mlt-bt05-ble-module-a-clone-of-a-clone/ , 
- https://blog.yavilevich.com/2017/04/fixing-a-bad-state-pin-on-an-mlt-bt05-ble-module/ , 
- https://blog.yavilevich.com/2018/04/should-you-throw-away-your-cc41-hm-10-clones-now-that-android-8-is-here/ 

![example screenshot](http://blog.yavilevich.com/wp-content/uploads/2016/12/ble_sketch_start.png)
