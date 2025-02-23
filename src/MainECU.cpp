/*
    Основная программа Main ECU модуля.
*/

#include <Arduino.h>

#include <StateDB.h>
#include <L2Wrapper.h>
#include <L3Wrapper.h>


StateDB DB;



L2Wrapper L2;

bool L2OnRX(L2Wrapper::packet_t &request, L2Wrapper::packet_t &response);
void L2OnError(int8_t code);



L3DriverBluetooth driver_ss;    // Для соединения по BT.
//L3DriverSerial driver_ss;     // Для соединения по Serial.
L3Wrapper L3(0, driver_ss);

bool L3OnRX(L3Wrapper::packet_t &request, L3Wrapper::packet_t &response);
void L3OnError(L3Wrapper::packet_t &packet, int8_t code);








#include <Emulator.h>
void EmulatorOnUpdate(uint32_t id, uint8_t *bytes, uint8_t length, uint32_t time)
{
    DB.Set(id, bytes, length, time);

    return;
}
Emulator em(EmulatorOnUpdate);
//								uint32_t id, T min, T max, uint16_t interval, T step, T value, algorithm_t algorithm
VirtualDevice<uint32_t> dev_voltage(174,		62000,		82000,		2500,		250,		74320,		VirtualDevice<uint32_t>::ALG_MINFADEMAX);
VirtualDevice<uint8_t>    dev_speed(125,		0,			101,		750,		1,			2,			VirtualDevice<uint8_t>::ALG_MINFADEMAX);
VirtualDevice<int32_t>  dev_current(239,		-150000,	150000,		1000,		250,		-1124,		VirtualDevice<int32_t>::ALG_RANDOM);
VirtualDevice<bool>       dev_light(513,		0,			1,			5000,		1,			0,			VirtualDevice<bool>::ALG_MINMAX);






void PrintArrayHex(uint8_t *data, uint8_t length, bool prefix = true)
{
    for(uint8_t i = 0; i < length; ++i)
    {
        if(prefix == true) Serial.print("0x");
        if(data[i] < 0x10) Serial.print("0");
        Serial.print(data[i], HEX);
        Serial.print(" ");
    }
    
    return;
}



void setup()
{
    Serial.begin(115200);
    Serial.println("Start Main ECU");

    
    L2.RegCallback(L2OnRX, L2OnError);
    L2.Init();
    
    L3.RegCallback(L3OnRX, L3OnError);
    L3.Init();




	em.RegDevice(dev_voltage);
	em.RegDevice(dev_speed);
	em.RegDevice(dev_current);
	em.RegDevice(dev_light);




    
    return;
}

uint32_t current_time = 0;
uint32_t tick = 0;

void loop()
{
    current_time = millis();

    L3.IncomingByte();
    
    
    
    em.Processing(current_time);
    
    

    return;
}


// Приём пакета по протоколу L3. Реализовано.
bool L3OnRX(L3Wrapper::packet_t &request, L3Wrapper::packet_t &response)
{
    bool result = false;
    
    // debug //
    uint8_t *packet_ptr = request.GetPacketPtr();
    uint8_t *data_ptr = request.GetDataPtr();

	Serial.print("RawPacket(");
	Serial.print(request.GetPacketLength());
	Serial.print("): ");
    PrintArrayHex(packet_ptr, request.GetPacketLength());
	Serial.println();
	
	Serial.print("Type: ");
	Serial.println( request.Type() );
	
	Serial.print("Param: ");
	Serial.println( request.Param() );
	
	Serial.print("RawData(");
	Serial.print(request.GetDataLength());
	Serial.print("): ");
    PrintArrayHex(data_ptr, request.GetDataLength());
	Serial.println();
    Serial.println();
    // debug //

    
    // https://wiki.starpixel.org/books/mainecu/page/protokol-l3#bkmrk-%D0%A2%D0%B8%D0%BF%D1%8B-%D0%B7%D0%B0%D0%BF%D1%80%D0%BE%D1%81%D0%B0
    switch (request.Type())
    {
        case 0x00:
        {
            // Все сервисные флаги будут установлены автоматически.
            response = request;
            result = true;
            
            break;
        }
        case 0x01:
        {
            StateDB::db_t db_obj;
            if( DB.Get(request.Param(), db_obj) == true )
            {
                response.Type( request.Type() );
                response.Param( request.Param() );
                response.PutData( db_obj.data, db_obj.length );
            }
            else
            {
                response.Type( 0x1E );
                response.Param( request.Param() );
                response.PutData( 0x01 );
            }
            result = true;
            
            break;
        }
        case 0x11:
        {
            if( DB.Set( request.Param(), data_ptr, request.GetDataLength(), request.GetPacketTime() ) == true )
            {
                StateDB::db_t db_obj;
                DB.Get(request.Param(), db_obj);
                
                response.Type( request.Type() );
                response.Param( request.Param() );
                response.PutData( db_obj.data, db_obj.length );
            }
            else
            {
                response.Type( 0x1E );
                response.Param( request.Param() );
                response.PutData( 0x03 );
            }
            result = true;
            
            break;
        }
        default:
        {
            response.Type( 0x1E );
            response.Param( request.Param() );
            response.PutData( 0x02 );
            result = true;
            
            break;
        }
    }
    
    return result;
}

// Ошибка приёма пакета по протоколу L3. Реализовано.
void L3OnError(L3Wrapper::packet_t &packet, int8_t code)
{
    
    uint8_t *packet_ptr = packet.GetPacketPtr();

	Serial.print("RawPacket(");
	Serial.print(packet.GetPacketLength());
	Serial.print("): ");
    PrintArrayHex(packet_ptr, packet.GetPacketLength());
	Serial.println();
    
    switch (code)
	{
		case packet.ERROR_FORMAT:
		{
			Serial.println("ERROR_FORMAT");
			
			break;
		}
		case packet.ERROR_VERSION:
		{
			Serial.println("ERROR_VERSION");
			
			break;
		}
		case packet.ERROR_CRC:
		{
			Serial.println("ERROR_CRC");
			
			break;
		}
		case packet.ERROR_OVERFLOW:
		{
			Serial.println("ERROR_OVERFLOW");
			
			break;
		}
		default:
		{
			break;
		}
	}

    Serial.println();
    Serial.println();
    
    return;
}







// Приём пакета по протоколу L2. Не реализовано.
bool L2OnRX(L2Wrapper::packet_t &request, L2Wrapper::packet_t &response)
{
    bool result = false;

    
    Serial.println("CAN RX: ");
    Serial.print(" > Address: "); Serial.print( request.address, HEX ); Serial.println(";");
    Serial.print(" > Length: "); Serial.print( request.length ); Serial.println(";");
    Serial.print(" > Data: "); PrintArrayHex( request.data, request.length ); Serial.println(";");
    Serial.println();
    
    
    return result;
}

// Ошибка приёма пакета по протоколу L2. Не реализовано.
void L2OnError(int8_t code)
{
    
    
    Serial.println("CAN ERROR: ");
    Serial.print(" > Code: "); Serial.print( code ); Serial.println(";");
    Serial.println();
    
    
    return;
}
