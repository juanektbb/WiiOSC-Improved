/* class to interact with wiiosc */
// TODO: add ir responders; add classic responders

WiiMoteOSC {
	var <id, <address;
	var <spec, <>action, <actionSpec; // <slots
	var <>closeAction, <>connectAction, <>disconnectAction;
	var <calibration;
	var <remote_led, <remote_buttons, <remote_motion, <remote_ir;
	var <nunchuk_buttons, <nunchuk_motion, <nunchuk_stick;
	var <classic_buttons, <classic_stick1, <classic_stick2, <classic_analog;
	var <>extension, <>battery;
	var <>dumpEvents = false;

	classvar all;
	classvar <responders;
	classvar < eventLoopIsRunning = false;
	classvar <>path = "/usr/local/bin/";
	classvar <appaddr;
	classvar pipe, piperout;
	
	*initClass {
		all = Array.fill( 4, 0 );
	}

	deviceSpec {
		^(
			ax: { remote_motion[0] },
			ay: { remote_motion[1] },
			az: { remote_motion[2] },
			ao: { remote_motion[3] },

			bA: { remote_buttons[0] },
			bB: { remote_buttons[1] },
			bOne: { remote_buttons[2] },
			bTwo: { remote_buttons[3] },
			bMinus: { remote_buttons[4] },
			bHome: { remote_buttons[5] },
			bPlus: { remote_buttons[6] },
			bUp: { remote_buttons[7] },
			bDown: { remote_buttons[8] },
			bLeft: { remote_buttons[9] },
			bRight: { remote_buttons[10] },

			px: { remote_ir[0] },
			py: { remote_ir[1] },
			angle: { remote_ir[2] },
			tracking: { remote_ir[3] },

			nax: { nunchuk_motion[0] },
			nay: { nunchuk_motion[1] },
			naz: { nunchuk_motion[2] },
			nao: { nunchuk_motion[3] },

			nsx: { nunchuk_stick[0] },
			nsy: { nunchuk_stick[1] },

			nbZ: { nunchuk_buttons[0] },
			nbC: { nunchuk_buttons[1] },

			cbX: { classic_buttons[0] },
			cbY: { classic_buttons[1] },
			cbA: { classic_buttons[2] },
			cbB: { classic_buttons[3] },
			cbL: { classic_buttons[4] },
			cbR: { classic_buttons[5] },
			cbZL: { classic_buttons[6] },
			cbZR: { classic_buttons[7] },
			cbUp: { classic_buttons[8] },
			cbDown: { classic_buttons[9] },
			cbLeft: { classic_buttons[10] },
			cbRight: { classic_buttons[11] },
			cbMinus: { classic_buttons[12] },
			cbHome: { classic_buttons[13] },
			cbPlus: { classic_buttons[14] },

			csx1: { classic_stick1[0] },
			csy1: { classic_stick1[1] },

			csx2: { classic_stick2[0] },
			csy2: { classic_stick2[1] },

			caleft: { classic_analog[0] },
			caright: { classic_analog[1] }
		)
	}

	*all {
		^all.copy
	}
	*new { |i,addr|
		^super.new.prInit(i,addr);
	}

	*makeResponders{
		responders = [
			OSCresponderNode(appaddr, "/wii/found", 
				{ |t, r, msg| ("WII found: "+msg[1] + msg[2] ).postln; 
					WiiMoteOSC.new( msg[1], msg[2] ); }),
			OSCresponderNode(appaddr, "/wii/connected", 
				{ |t, r, msg| ("WII connected" +  msg[1]).postln; 
					all[msg[1]].connectAction.value; }),
			OSCresponderNode(appaddr, "/wii/disconnected", 
				{ |t, r, msg| ("WII disconnected: "+msg[1]).postln; 
					all[msg[1]].disconnectAction.value;  }),
			OSCresponderNode(appaddr, "/wii/error", 
				{ |t, r, msg| ("WII error: "+msg).postln; 
					//	all[msg[1]].disconnectAction.value;  
				}),
			OSCresponderNode(appaddr, '/wii/acc/x', 
				{ |t, r, msg| 
					all[msg[1]].actionSpec.at( \ax ).value( msg[2] );
					all[msg[1]].remote_motion[0] = msg[2]; 
				}),
			OSCresponderNode(appaddr, '/wii/acc/y', 
				{ |t, r, msg| 
					all[msg[1]].actionSpec.at( \ay ).value( msg[2] );  
					all[msg[1]].remote_motion[1] = msg[2]; 
				}),
			OSCresponderNode(appaddr, '/wii/acc/z', 
				{ |t, r, msg| 
					all[msg[1]].actionSpec.at( \az ).value( msg[2] );  
					all[msg[1]].remote_motion[2] = msg[2]; 
				}),
			OSCresponderNode(appaddr, '/wii/nunchuk/acc/x', 
				{ |t, r, msg| 
					all[msg[1]].actionSpec.at( \nax ).value( msg[2] );  
					all[msg[1]].nunchuk_motion[0] = msg[2]; 
				}),
			OSCresponderNode(appaddr, '/wii/nunchuk/acc/y', 
				{ |t, r, msg| 
					all[msg[1]].actionSpec.at( \nay ).value( msg[2] );  
					all[msg[1]].nunchuk_motion[1] = msg[2]; 
				}),
			OSCresponderNode(appaddr, '/wii/nunchuk/acc/z', 
				{ |t, r, msg| 
					all[msg[1]].actionSpec.at( \naz ).value( msg[2] );  
					all[msg[1]].nunchuk_motion[2] = msg[2]; 
				}),
			OSCresponderNode(appaddr, '/wii/nunchuk/joy/x', 
				{ |t, r, msg| 
					all[msg[1]].actionSpec.at( \nsx ).value( msg[2] );  
					all[msg[1]].nunchuk_stick[0] = msg[2]; 
				}),
			OSCresponderNode(appaddr, '/wii/nunchuk/joy/y', 
				{ |t, r, msg| 
					all[msg[1]].actionSpec.at( \nsy ).value( msg[2] );
					all[msg[1]].nunchuk_stick[1] = msg[2]; 
				}),
			OSCresponderNode(appaddr, '/wii/nunchuk/keys/z', 
				{ |t, r, msg| 
					all[msg[1]].actionSpec.at( \nbZ ).value( msg[2] ); 
					all[msg[1]].nunchuk_buttons[0] = msg[2]; 
				}),
			OSCresponderNode(appaddr, '/wii/nunchuk/keys/c', 
				{ |t, r, msg| 
					all[msg[1]].actionSpec.at( \nbC ).value( msg[2] ); 
					all[msg[1]].nunchuk_buttons[1] = msg[2]; 
				}),

			OSCresponderNode(appaddr, '/wii/keys/one', 
				{ |t, r, msg| 
				all[msg[1]].actionSpec.at( \bOne ).value( msg[2] ); 
				all[msg[1]].remote_buttons[2] = msg[2]; 
			}),
			OSCresponderNode(appaddr, '/wii/keys/two', 
				{ |t, r, msg| 
					all[msg[1]].actionSpec.at( \bTwo ).value( msg[2] );
					all[msg[1]].remote_buttons[3] = msg[2]; 
				}),
			OSCresponderNode(appaddr, '/wii/keys/a', 
				{ |t, r, msg| 
					all[msg[1]].actionSpec.at( \bA ).value( msg[2] ); 
					all[msg[1]].remote_buttons[0] = msg[2]; 
				}),
			OSCresponderNode(appaddr, '/wii/keys/b', 
				{ |t, r, msg|
					all[msg[1]].remote_buttons[1] = msg[2]; 
					all[msg[1]].actionSpec.at( \bB ).value( msg[2] ); 
				}),
			OSCresponderNode(appaddr, '/wii/keys/minus', 
				{ |t, r, msg| 
					all[msg[1]].actionSpec.at( \bMinus ).value( msg[2] ); 
					all[msg[1]].remote_buttons[4] = msg[2]; 
				}),
			OSCresponderNode(appaddr, '/wii/keys/home', 
				{ |t, r, msg| 
					all[msg[1]].actionSpec.at( \bHome ).value( msg[2] ); 
					all[msg[1]].remote_buttons[5] = msg[2]; 
				}),
			OSCresponderNode(appaddr, '/wii/keys/plus', 
				{ |t, r, msg| 
					all[msg[1]].actionSpec.at( \bPlus ).value( msg[2] ); 
					all[msg[1]].remote_buttons[6] = msg[2]; 
				}),
			OSCresponderNode(appaddr, '/wii/keys/up', 
				{ |t, r, msg| 
					all[msg[1]].actionSpec.at( \bUp ).value( msg[2] ); 
					all[msg[1]].remote_buttons[7] = msg[2]; 
				}),
			OSCresponderNode(appaddr, '/wii/keys/down', 
				{ |t, r, msg| 
					all[msg[1]].actionSpec.at( \bDown ).value( msg[2] ); 
					all[msg[1]].remote_buttons[8] = msg[2]; 
				}),
			OSCresponderNode(appaddr, '/wii/keys/left', 
				{ |t, r, msg| 
					all[msg[1]].actionSpec.at( \bLeft ).value( msg[2] ); 
					all[msg[1]].remote_buttons[9] = msg[2]; 
				}),
			OSCresponderNode(appaddr, '/wii/keys/right', 
				{ |t, r, msg| 
					all[msg[1]].actionSpec.at( \bRight ).value( msg[2] ); 
					all[msg[1]].remote_buttons[10] = msg[2]; 
				}),
			OSCresponderNode(appaddr, '/wii/battery', 
					{ |t, r, msg| 
						all[msg[1]].battery = msg[2];
					}),
			OSCresponderNode(appaddr, '/wii/extension', { 
				|t, r, msg| 
				all[msg[1]].extension = msg[2]; 
				("extension:"+msg[1]+msg[2]).postln;
			})
		];
		
		responders.do{ |it| it.add };
	}

	*clearResponders{
		responders.do{ |it| it.remove };
		responders = [];
	}

	setAction{ |key,keyAction|
		actionSpec.put( key, keyAction );
	}
	removeAction{ |key|
		actionSpec.removeAt( key );
	}
	at { | controlName |
		^this.spec.atFail(controlName, {
			Error("invalid control name").throw
		});
	}
	getLEDState { |id|
		^remote_led[id]
	}
	setLEDState { |lid,state|
		remote_led[lid] = state;
		appaddr.sendMsg( "/wii/leds", id, remote_led[0], remote_led[1], remote_led[2], remote_led[3] ) 
	}
	
	rumble{ |onoff|
		appaddr.sendMsg( "/wii/rumble", id, onoff );
	}

	/*	updatetime_{ |updt|
		appaddr.sendMsg( "/wii/updatetime", updt );
		}*/
	/*
	getExtension{
		appaddr.sendMsg( "/wii/extension", id );
		}*/

	enable{ |onoff|
		appaddr.sendMsg( "/wii/enable", id, onoff );
	}

	enableReport{ |onoff|
		appaddr.sendMsg( "/wii/enable/report", id, onoff );
	}

	enableIR{ |onoff|
		appaddr.sendMsg( "/wii/enable/ir", id, onoff );
	}

	enableMotion{ |onoff|
		appaddr.sendMsg( "/wii/enable/motion", id, onoff );
	}

	enableButtons{ |onoff|
		appaddr.sendMsg( "/wii/enable/button", id, onoff );
	}

	enableExtension{ |onoff|
		appaddr.sendMsg( "/wii/enable/extension", id, onoff );
	}

		/*	playSample{ |play,freq,vol,sample=0|
			this.prPlaySpeaker( play, freq, vol, sample );
			}
			mute{ |onoff|
			this.prMuteSpeaker( onoff );
			}
			enableSpeaker{ |onoff|
			this.prEnableSpeaker( onoff );
			}
			initSpeaker{ |format=0|
			this.prInitSpeaker( format );
			}*/


	*start{ 
		var command;
		this.makeResponders;
		command = (path++"wiiosc \""++NetAddr.langPort.asString++"\"");
		command.unixCmd;
		command.postln;

		/*	pipe = Pipe.new(command, "r"); 
		piperout = Routine{ var l;
			loop{
				l = pipe.getLine; // get the first line
				while({l.notNil}, {l.postln; l = pipe.getLine; });
				1.0.wait } };
		piperout.play;
		*/

		appaddr = NetAddr.new( "localhost", 57150 );
		UI.registerForShutdown({
			this.stop;
		});
	}

	*info{
		appaddr.sendMsg( "/wii/info" );
	}

	*discover{
		appaddr.sendMsg( "/wii/discover" );
	}

	*stop{
		//piperout.stop;
		//pipe.close;
		appaddr.sendMsg( "/wii/quit" );
		this.clearResponders;
	}

	// PRIVATE
	prInit { |i,addr|
		id = i;
		address = addr;
		remote_led = Array.fill( 4, 0 );
		remote_buttons = Array.fill( 11, 0 );
		remote_motion = Array.fill( 4, 0 );
		remote_ir = Array.fill( 4, 0 );
		nunchuk_buttons = Array.fill( 2, 0 );
		nunchuk_motion = Array.fill( 4, 0 );
		nunchuk_stick = Array.fill( 2, 0 );
		classic_buttons = Array.fill( 15, 0 );
		classic_stick1 = Array.fill( 2, 0 );
		classic_stick2 = Array.fill( 2, 0 );
		classic_analog = Array.fill( 2, 0 );

		//		this.prOpen;
		all[id] = this;
		closeAction = {};
		connectAction = {};
		disconnectAction = {};
		//	this.prWiiGetLED( remote_led );
		//	calibration = this.prCalibration(WiiCalibrationInfo.new);
 		spec = this.deviceSpec;
		actionSpec = IdentityDictionary.new;
	}
}
