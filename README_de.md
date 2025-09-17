# Loco-CAN Software

@author: Thomas H Winkler
@copyright: 2020-2025
@lizence: GG0

Loco-CAN ist ein bussystem für personenbefördernde Gartenbahnen. Es basiert auf dem CAN-bus, der in den 1980er Jahren für Autos entwickelt wurde, um den Verkabelungsaufwand zu reduzieren. Die Kommunikation auf der Zweidrahtleitung ist sehr robust gegen Störungen unempfindlich. Das Loco-CAN system ist sehr modular, vereinfacht den elektrischen Aufbau in Gartenbahnfahrzeugen und bietet zahlreiche Funktionen.

## File Struktur

Die Software für alle möglichen Module ist in einem einzigen Projekt untergebracht. Die Auswahl, welche konkrete Variante compiliert werden soll, erfolgt über eine Konfigurationsdatei, in der auch die verwendete Hardwareversion bestimmt werden kann. Die Grundfunktionen werden von allen Hardwareversionen ab 1.0 untertützt, lediglich ein USB-Anschluss oder eine WiFi-Anbindung bleiben der ESP32-Familie vorbehalten.

Das Projekt liegt in folgender Filestruktur:

	Loco-CAN
	- doc
	- src
		- core
		- function
		- module
		can_protocol.h
		LocoCANcore.cpp
		LocoCANcore.h
	config.h
	loco-can-software.ino
	README.md

### doc

Das doc-Verzeichnis enthält alle nötigen Dokumentation, ist im derzeitigen Entwicklungsstadium noch eine Sammelstelle unterschiedlicher Inhalte.

### src

Das src-Verzeichnis enthällt sämtlichen Code des Projekts. Es ist in drei Unterbereiche geteilt:

	- core
	- function
	- module

Das .ino-File erstellt den LocoCANcore, der sich hier befindet. Mit der core-Klasse wird die Software-Laufzeitumgebung geschaffen, alle nötigen Funktionen eingebunden und gestartet.

#### core

Das core-Verzeichnis enthällt alle globalen Methoden die für die Module genutzt werden können. Zentral ist die CAN-Kommunikation. Die weiteren Methoden dienen einer vereinheitlichten Ansteuerung der Hardwareelemente und zur Kontrolle des Programmablaufs. Folgende Methoden stehen zur Verfügung:

	analogSwitch
	button
	can
	hash
	led
	measure
	pwm
	servo
	timeout
	uniqueID
	value
	watchdog

#### function

Funktionen sind komplexe Methoden, die einen bestimmten Ablauf im Loco-CAN system übernehmen. Das kann ein Kontroller sein, an den Potentiometer und Schalter angeschlossen sind und damit eine Lokomotive gesteuert werden kann, ein Interface, um eine Motorenstufe korrekt und sicher ansteuern zu können oder die Funktion Schalter mit definierten CAN-Nachrichten schalten zu können.

Ein Modul kann eine Funktion nutzen, es ist aber auch möglich mehrere Funktionen auf einer einzigen Hardware zu kombinieren. In diesem Fall läuft die Kommunikation zwischen den Funktionen über normale CAN-Nachrichten innerhalb der Software, bevor sie auf den externen Bus weitergeleitet werden.

Funktionen können leicht erweitert werden. Dazu gibt es im doc-Verzeichnis Beispiele, die die Grundstruktur beschreiben und als Vorlage dienen können.

Eine Funktion muss an folgenden Stellen eingetragen werden, um genutzt werden zu können:

**src/LocoCANcore.h**

	#ifdef {FUNCTION_NAME_H}
		{FUNCTION_NAME} _{function_name};
	#endif

**src/LocoCANcore.cpp**

	void LocoCANcore::begin(void) {

		...

		// =========================
		// start function {FUNCTION_NAME}
		#ifdef {FUNCTION_NAME_H}
			_{function_name}.begin(func++);
		#endif

		...
	}

	void LocoCANcore::update(void) { 

		...

		// ==========================
		// update function {FUNCTION_NAME}
		#ifdef {FUNCTION_NAME_H}
			_{function_name}.update(can_message);
		#endif
	
		...
	}

#### module

Ein Modul repräsentiert eine Hardware mit gewissen Parametern, wie der verwendeten Architektur, den Ein- und Ausgängen und spezialisierten Elektronikkomponenten. Es wird in einem h-File definiert. Die Auswahl, welche Moduldatei genutzt wird, geschieht am Ende des config.h Files. Dort wird der Modulname und die Modulversion angegeben.

Für jede Hardwareversion gibt es einen Definitionsblock, die sich beim Umfang der Funktionen unterscheiden können.

	#pragma once

	/* ******************************************
	 * The module version is set in the config.h file
	 */

	#ifdef MODULE_VERSION = {module_version}

		// ======================================
		// BASIC SETTINGS
		// ======================================

		#define MODULE_PLATFORM_ATMEGA
		#define PLATFORM_ANALOG_RESOLUTION 1024

		#define CAN_RX 10
		#define CAN_TX 2
		#define CAN_STATUS_LED 5
		#define CAN_MAX_FILTER 8
		#define CAN_BUFFER_SIZE 8

		// ======================================
		// INCLUDED FUNCTIONS
		// ======================================

		/* define corresponding parameters */ 

		#include ".../function/{function_name}/main.h"
		...

	#endif

Der Block BASIC SETTINGS muss vorhanden sein. In ihm werden Grundeinstellungen für die core-Methoden vorgenommen. Im INCLUDED FUNCTIONS Block werden alle Funktionen eingebunden, die in dem Modul verwendet werden. Für jede Funktion müssen die nötigen Parameter folgen.

Ein Modul wird im config.h FIle definiert.

**config.h**

	#define {module_name} "{path to module header file}"
