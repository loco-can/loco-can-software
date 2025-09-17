# Function example

Diese Beispiel dient als Vorlage, um eine neue Funktion zu erstellen. Kopieren Sie dazu die Dateien in diesem Verzeichnis in ein neues Verzeichnis auf src/function und benennen es nach dem Funktionsnamen.

Jede Funktion muss zwei Methoden bieten:

	void begin(uint8_t func_id);
	void update(CAN_MESSAGE message);

## begin

Der Loco-CAN core bindet die Funktion ein, wenn sie im Modul definiert wurde. Der core startet sie und übergibt automatisch einen function-id. Dieser id muss bei jeder Nachricht, die von der Funktion gesendet wird, in der Variable func mitgegeben werden:

	struct CAN_MESSAGE {
	    uint32_t id;
	    uint16_t uuid;
	    uint8_t data[8];
	    uint8_t size;
	    uint8_t func;
	};

## update

Die update Methode aller Funktionen wird bei jedem Look aufgerufen. Es wird die CAN-Nachricht übergeben, die vom externen Bus gelesen wurde. Gibt es keine neue Nachricht, enthält der UUID der Nachricht den Wert Null. In dem Fall muss die Nachricht verworfen werden.