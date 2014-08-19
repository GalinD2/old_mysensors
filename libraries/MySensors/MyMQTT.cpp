/*
The MySensors library adds a new layer on top of the RF24 library.
It handles radio network routing, relaying and ids.

Created by Henrik Ekblad <henrik.ekblad@gmail.com>
Modified by Daniel Wiegert
This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
version 2 as published by the Free Software Foundation.
*/

#include "MyMQTT.h"

char broker[] PROGMEM = MQTT_BROKER_PREFIX;
char S_0[] PROGMEM = "Temperature";
char S_1[] PROGMEM = "Humidity";
char S_2[] PROGMEM = "Light";
char S_3[] PROGMEM = "Dimmer";
char S_4[] PROGMEM = "Pressure";
char S_5[] PROGMEM = "Forecast";
char S_6[] PROGMEM = "Rain";
char S_7[] PROGMEM = "Rainrate";
char S_8[] PROGMEM = "Wind";
char S_9[] PROGMEM = "Gust";
char S_10[] PROGMEM = "Direction";
char S_11[] PROGMEM = "UV";
char S_12[] PROGMEM = "Weight";
char S_13[] PROGMEM = "Distance";
char S_14[] PROGMEM = "Impedance";
char S_15[] PROGMEM = "Armed";
char S_16[] PROGMEM = "Tripped";
char S_17[] PROGMEM = "Watt";
char S_18[] PROGMEM = "KWH";
char S_19[] PROGMEM = "Scene_ON";
char S_20[] PROGMEM = "Scene_OFF";
char S_21[] PROGMEM = "Heater";
char S_22[] PROGMEM = "Heater_SW";
char S_23[] PROGMEM = "LightLVL";
char S_24[] PROGMEM = "Var1";
char S_25[] PROGMEM = "Var2";
char S_26[] PROGMEM = "Var3";
char S_27[] PROGMEM = "Var4";
char S_28[] PROGMEM = "Var5,";
char S_29[] PROGMEM = "Up";
char S_30[] PROGMEM = "Down";
char S_31[] PROGMEM = "Stop";
char S_32[] PROGMEM = "IR_Send";
char S_33[] PROGMEM = "IR_Receive";
char S_34[] PROGMEM = "Flow";
char S_35[] PROGMEM = "Volume";
char S_36[] PROGMEM = "Lock_Status";
char S_37[] PROGMEM = "Volt_Batt";  // Does not exist in mysensors lib!
char S_38[] PROGMEM = "";
char S_39[] PROGMEM = "";
char S_40[] PROGMEM = "";
char S_41[] PROGMEM = "";
char S_42[] PROGMEM = "";
char S_43[] PROGMEM = "";
char S_44[] PROGMEM = "";
char S_45[] PROGMEM = "";
char S_46[] PROGMEM = "";
char S_47[] PROGMEM = "";
char S_48[] PROGMEM = "";
char S_49[] PROGMEM = "Sketch_name"; // Does not exist in mysensors lib!
char S_50[] PROGMEM = "Sketch_version"; // Does not exist in mysensors lib!


PROGMEM const char *sType[] =
{ S_0, S_1, S_2, S_3, S_4, S_5, S_6, S_7, S_8, S_9, S_10, 
	S_11, S_12, S_13, S_14, S_15, S_16, S_17, S_18, S_19, S_20,
	S_21, S_22, S_23, S_24, S_25, S_26, S_27, S_28, S_29, S_30, 
	S_31, S_32, S_33, S_34, S_35, S_36, S_37, S_38, S_39, S_40,
	S_41, S_42, S_43, S_44, S_45, S_46, S_47, S_48, S_49, S_50};

	
MyMQTT::MyMQTT(uint8_t _intpin, uint8_t _cspin) : 
	MySensor(_intpin, _cspin) {
}

void MyGateway::begin(uint8_t paLevel, uint16_t frequency, RH_RF69::ModemConfigChoice modemChoice, void (*inDataCallback)(char *)) {
	Serial.begin(BAUD_RATE);
	isGateway = true;
	MQTTClient = false;
	
	if (inDataCallback != NULL) {
		useWriteCallback = true;
		dataCallback = inDataCallback;
	} 
	else {
		useWriteCallback = false;
	}

	nc.nodeId = 0;
	nc.distance = 0;

	// Start up the radio library
	setupRadio(paLevel, channel, modemChoice);
	// Send startup log message on serial
	//Serial.print(PSTR("Started\n"));//TODO: progmem gives error..? error: sType causes a section type conflict with __c
	Serial.print("Started\n");//TODO: fix this...
}

void MyMQTT::processRadioMessage() {
	if (process()) {
		// A new message was received from one of the sensors
		MyMessage message = getLastMessage();
		// Pass along the message from sensors to serial line
		SendMQTT(message);
	}

}

void MyMQTT::processMQTTMessage(char *inputString, int inputPos) {
	char *str, *p, *value=NULL;
	char i = 0;
	buffer[0]= 0;
	buffsize = 0;
#ifdef TCPDUMP
	Serial.print("<<"); 
	char buf[4];
	for (int a=0; a<inputPos; a++) { sprintf(buf,"%02X ",(byte)inputString[a]); Serial.print(buf); } Serial.println("");
#endif
	if ((byte)inputString[0] >> 4 == MQTTCONNECT) {
		buffer[buffsize++] = MQTTCONNACK << 4;
		buffer[buffsize++] = 0x02;                  // Remaining length
		buffer[buffsize++] = 0x00;                  // Connection accepted
		buffer[buffsize++] = 0x00;                  // Reserved
		MQTTClient=true;
	}
	if ((byte)inputString[0] >> 4 == MQTTPINGREQ) {
		buffer[buffsize++] = MQTTPINGRESP << 4;
		buffer[buffsize++] = 0x00;
	}
	if ((byte)inputString[0] >> 4 == MQTTSUBSCRIBE) {
		buffer[buffsize++] = MQTTSUBACK << 4;       // Just ack everything, we actually dont really care!
		buffer[buffsize++] = 0x03;                  // Remaining length
		buffer[buffsize++] = (byte)inputString[2];  // Message ID MSB
		buffer[buffsize++] = (byte)inputString[3];  // Message ID LSB
		buffer[buffsize++] = MQTTQOS0;              // QOS level
	}
	if ((byte)inputString[0] >> 4 == MQTTUNSUBSCRIBE) {
		buffer[buffsize++] = MQTTUNSUBACK << 4;
		buffer[buffsize++] = 0x02;                  // Remaining length
		buffer[buffsize++] = (byte)inputString[2];  // Message ID MSB
		buffer[buffsize++] = (byte)inputString[3];  // Message ID LSB
	}
	if ((byte)inputString[0] >> 4 == MQTTDISCONNECT) {
		MQTTClient=false;
	}
	if (buffsize > 0) {
#ifdef TCPDUMP
		Serial.print(">>"); 
		char buf[4];
		for (int a=0; a<buffsize; a++) { sprintf(buf,"%02X ",(byte)buffer[a]);  Serial.print(buf); } Serial.println("");    
#endif
		dataCallback(buffer,&buffsize);
	}
	// We publish everything we get, we dont care if its subscribed or not!
	if ((byte)inputString[0] >> 4 == MQTTPUBLISH || (MQTT_SEND_SUBSCRIPTION && (byte)inputString[0] >> 4 == MQTTSUBSCRIBE)) { 
		buffer[0]= 0;
		buffsize = 0;
		if ((byte)inputString[0] >> 4 == MQTTSUBSCRIBE) {
			strncat(buffer,inputString+6,inputString[5]);
		} else {
			strncat(buffer,inputString+4,inputString[3]);
		}
		msg.sender = GATEWAY_ADDRESS;
		for (str = strtok_r(buffer, "/", &p);       // split using semicolon
		str && i < 4; str = strtok_r(NULL, "/", &p)  //get subsequent tokens (?) 
		) {
			switch (i) {
			case 0: // Radioid (destination)
				if (strcmp_P(str,broker)!=0) {
					return; //Message not for us or malformatted!
				}
				break;
			case 1:
				msg.destination = atoi(str);
				break;
			case 2:
				msg.sensor = atoi(str);
				break;
			case 3:
				char match=0;
				for (int j=0; strcpy_P(convBuf, (char*)pgm_read_word(&(sType[j]))) ; j++) {
					if (strcmp(str,convBuf)==0) { 
						match=j; 
						break;
					}
				} 
				msg.type = match;
				break;
			}
			i++;
		}
		if ((char)inputString[1] > (char)(inputString[3]+2) && !((byte)inputString[0] >> 4 == MQTTSUBSCRIBE)) {
			strcpy(convBuf,inputString+(inputString[3]+4));
			msg.set(convBuf);
		} else { 
			msg.set("");
		}
		msg.sender = GATEWAY_ADDRESS;
		mSetCommand(msg,C_SET);
		mSetRequestAck(msg,false);
		mSetAck(msg,false);
		mSetVersion(msg, PROTOCOL_VERSION);
		sendRoute(msg);	
	}
}


void MyMQTT::SendMQTT(MyMessage &msg) {
//serial(PSTR("%d;%d;%d;%d;%s\n"),msg.sender, msg.sensor, mGetCommand(msg), msg.type, msg.getString(convBuf));
	buffsize = 0;
	if (!MQTTClient) return;
	if (msg.isAck()) {
Serial.println("msg is ack!");
		if (msg.sender==255 && mGetCommand(msg)==C_INTERNAL && msg.type==I_ID_REQUEST) {
			// TODO: sending ACK request on id_response fucks node up. doesn't work. 
			// The idea was to confirm id and save to EEPROM_LATEST_NODE_ADDRESS.
		}
	} else {
		// we have to check every message if its a newly assigned id or not.
		// Ack on I_ID_RESPONSE does not work, and checking on C_PRESENTATION isn't reliable.
		char newNodeID = loadState(EEPROM_LATEST_NODE_ADDRESS)+1;
		if (newNodeID <= MQTT_FIRST_SENSORID) newNodeID = MQTT_FIRST_SENSORID;
		if (msg.sender==newNodeID) {
			saveState(EEPROM_LATEST_NODE_ADDRESS,newNodeID);
		}
		if (mGetCommand(msg)==C_INTERNAL && msg.type==I_CONFIG) {  // CONFIG
			// As for now there is only one 'config' request. 
			// We force SI! Resistance is futile!
			//
			// Todo : Support for more config types, Maybe just read from own EEPROM? 
			// 			Use internal EEPROM_CONTROLLER_CONFIG_ADDRESS and special MQTT address to write to 
			//			EEPROM_CONTROLLER_CONFIG_ADDRESS & EEPROM_LOCAL_CONFIG_ADDRESS
			msg.destination = msg.sender; //NodeID
			msg.sender = GATEWAY_ADDRESS;
			msg.sensor = 255;	  		//SensorID
			msg.type = I_CONFIG;		//msgType
			mSetCommand(msg,C_INTERNAL);  //Subtype
			mSetRequestAck(msg,false);
			mSetAck(msg,false);
			msg.set("M");
			mSetVersion(msg, PROTOCOL_VERSION);
			sendRoute(msg);
		} else if (mGetCommand(msg)==C_PRESENTATION && (msg.type==S_ARDUINO_NODE || msg.type==S_ARDUINO_REPEATER_NODE)) {
			//Doesnt work to check new sensorID here, this message does not always arrive.. See above.
		} else if (msg.sender==255 && mGetCommand(msg)==C_INTERNAL && msg.type==I_ID_REQUEST) {
			char newNodeID = loadState(EEPROM_LATEST_NODE_ADDRESS)+1;
			if (newNodeID <= MQTT_FIRST_SENSORID) newNodeID = MQTT_FIRST_SENSORID;
			if (newNodeID >= MQTT_LAST_SENSORID) return; // Sorry no more id's left :(
			msg.destination = msg.sender; 		//NodeID
			msg.sender = GATEWAY_ADDRESS;
			msg.sensor = 255;	  				//SensorID
			msg.type = I_ID_RESPONSE;			//MsgType
			mSetCommand(msg,C_INTERNAL); 		//Subtype
			mSetRequestAck(msg,false);			//Request ack doesn't work, node/gateway gets stuck in enless loop.
			mSetAck(msg,false);
			msg.set((uint8_t)newNodeID);					//Payload
			mSetVersion(msg, PROTOCOL_VERSION);
			sendRoute(msg);
//			if (sendRoute(msg)) saveState(EEPROM_LATEST_NODE_ADDRESS,newNodeID);  // If send OK save to eeprom. DOES NOT ALWAYS RETURN 'OK'!?
		} else if (mGetCommand(msg)!=0) {
			if (mGetCommand(msg)==3) msg.type=msg.type+38;
			buffer[buffsize++] = MQTTPUBLISH << 4;	    // 0: 
			buffer[buffsize++] = 0x09;                  // 1: Remaining length with no payload, we'll set this later to correct value, buffsize -2
			buffer[buffsize++] = 0x00;                  // 2: Length MSB (Remaing length can never exceed ff,so MSB must be 0!)
			buffer[buffsize++] = 0x08;				    // 3: Length LSB (ADDR), We'll set this later
			strcpy_P(buffer+4, broker);
			buffsize+=strlen_P(broker);
			buffsize+=sprintf(&buffer[buffsize],"/%i/%i/",msg.sender,msg.sensor);
			buffsize+=strncpysType_retL(buffer,msg.type,buffsize);
			buffer[3]=buffsize-4;						// Set correct address length on byte 4.
#ifdef DEBUG
			Serial.println((char*)&buffer[4]);
#endif
			msg.getString(convBuf);
			for (int a=0; a<strlen(convBuf); a++) {		// Payload
				buffer[buffsize++] = convBuf[a];
			}
			buffer[1]=buffsize-2;						// Set correct Remaining length on byte 2.

#ifdef TCPDUMP
			Serial.print(">>");
			char buf[4];
			for (int a=0; a<buffsize; a++) { sprintf(buf,"%02X ",(byte)buffer[a]);  Serial.print(buf); } Serial.println("");    
#endif
			dataCallback(buffer,&buffsize);
		}
	}	
}

char MyMQTT::strncpysType_retL(char *str, char index, char start) {
	char c;
	char l;
	char *p = (char *)pgm_read_word(&(sType[index])); 
	str+=start;
	while ((c = pgm_read_byte(p))) {
		*str=c;
		str++;
		p++;
		l++;
	}
	*str=0;
	return l;
}
