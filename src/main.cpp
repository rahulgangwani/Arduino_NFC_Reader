#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_PN532.h>

// If using the breakout or shield with I2C, define just the pins connected
// to the IRQ and reset lines.  Use the values below (2, 3) for the shield!
#define PN532_IRQ   (2)
#define PN532_RESET (3)  // Not connected by default on the NFC Shield

// Or use this line for a breakout or shield with an I2C connection:
Adafruit_PN532 nfc(PN532_IRQ, PN532_RESET);

bool reader_failed = false;

// put function declarations here:
String getPageString(const byte *data, const uint32_t numBytes);
bool parsePacket(String initialString, String &parsedString);

void setup() {
  Serial.begin(9600);
  nfc.begin();

  uint32_t versiondata = nfc.getFirmwareVersion();
  if (! versiondata) {
    Serial.print("Didn't find PN53x board");
    reader_failed = true;
  }
  // Got ok data, print it out!
  if (!reader_failed)
  {
    Serial.print("Found chip PN5"); Serial.println((versiondata>>24) & 0xFF, HEX);
    Serial.print("Firmware ver. "); Serial.print((versiondata>>16) & 0xFF, DEC);
    Serial.print('.'); Serial.println((versiondata>>8) & 0xFF, DEC);
  }
  else
  {
    Serial.println("Failed to find NFC reader!!");
  }

  Serial.println("Waiting for an ISO14443A Card ...");
}

void loop() {
  uint8_t success;
  uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the returned UID
  uint8_t uidLength;                        // Length of the UID (4 or 7 bytes depending on ISO14443A card type)
  bool gotPacket = false;
  String received_text = "";
  String packet;

  // Wait for an NTAG203 card.  When one is found 'uid' will be populated with
  // the UID, and uidLength will indicate the size of the UUID (normally 7)
  if (!reader_failed) {
    success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);
  }

  if (success) {
    // Display some basic information about the card
    Serial.println("Found an ISO14443A card");
    Serial.print("  UID Length: ");Serial.print(uidLength, DEC);Serial.println(" bytes");
    Serial.print("  UID Value: ");
    nfc.PrintHex(uid, uidLength);
    Serial.println("");

    if (uidLength == 7)
    {
      uint8_t data[32];
      uint8_t customText[20];  //For now, it's this size, but that's based on max packet size

      // We probably have an NTAG2xx card (though it could be Ultralight as well)
      Serial.println("Seems to be an NTAG2xx tag (7 byte UID)");

      // NTAG2x3 cards have 39*4 bytes of user pages (156 user bytes),
      // starting at page 4 ... larger cards just add pages to the end of
      // this range:

      // See: http://www.nxp.com/documents/short_data_sheet/NTAG203_SDS.pdf

      // TAG Type       PAGES   USER START    USER STOP
      // --------       -----   ----------    ---------
      // NTAG 203       42      4             39
      // NTAG 213       45      4             39
      // NTAG 215       135     4             129
      // NTAG 216       231     4             225

      bool startCollecting = false;
      for (uint8_t i = 0; i < 42; i++)
      {
        success = nfc.ntag2xx_ReadPage(i, data);

        // // Display the current page number
        // Serial.print("PAGE ");
        // if (i < 10)
        // {
        //   Serial.print("0");
        //   Serial.print(i);
        // }
        // else
        // {
        //   Serial.print(i);
        // }
        // Serial.print(": ");

        // Display the results, depending on 'success'
        if (success)
        {
          received_text += getPageString(data, 4);
        }
        else
        {
          Serial.println("Unable to read the requested page!");
        }
      }
      if (parsePacket(received_text, packet)) {
        Serial.print("Received Text: ");
        Serial.println(packet);
      }
    }
    else
    {
      Serial.println("This doesn't seem to be an NTAG203 tag (UUID length != 7 bytes)!");
    }
    Serial.flush();
    delay(5000);  //5 second delay to avoid overloading the serial buffer
  }
}

String getPageString(const byte *data, const uint32_t numBytes)
{
  String returnString = "";

  for (uint32_t i = 0; i < numBytes; i++) {
    if (data[i] > 31 && data[i] < 127)  //Within a valid ASCII range
    returnString += (char)data[i];
  }

  return returnString;
}

bool parsePacket(String initialString, String &parsedString) {
  int startIndex, endIndex;

  startIndex = initialString.indexOf("#!");
  endIndex = initialString.indexOf("!#");

  if ((startIndex == -1 || endIndex == -1) || (startIndex >= endIndex)) {
    parsedString = "";
    return false;
  }
  else {
    parsedString = initialString.substring(startIndex+2, endIndex);
    return true;
  }
}