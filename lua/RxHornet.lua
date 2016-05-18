
local function draw_status(x,y,src)

   local msg_table = {
      [0]  = "OFF",
      [1]  = "STANDBY",
      [23] = "PREHEAT",
      [15] = "BURNER ON",
      [5]  = "FUEL IGNIT",
      [25] = "BURN OUT",
      [7]  = "RAMP UP",
      [26] = "STEADY",
      [11] = "CALIBRATE",
      [18] = "WAIT ACC",
      [13] = "GO IDLE",
      [12] = "CAL IDLE",
      [2]  = "AUTO",
      [16] = "AUTO HC",
      [9]  = "SLOW DOWN",
      [10] = "COOL DOWN",
   }

   lcd.drawText(x+30,y+2,"STATUS",0)
   lcd.drawText(x+4,y+12,msg_table[getValue(src)],MIDSIZE)
end

local function draw_rpm(x,y,src)
   local rpm = getValue(src)
   lcd.drawPixmap(x+4,y+3,"/SCRIPTS/BMP/rpm.bmp")
   lcd.drawText(x+32,y+9,"RPM",0)
   lcd.drawNumber(x+50,y+19,rpm,MIDSIZE)
end

local function draw_egt(x,y,src)
   local egt = getValue(src)
   lcd.drawPixmap(x+7,y+7,"/SCRIPTS/BMP/temp.bmp")
   lcd.drawText(x+32,y+4,"EGT",0)
   lcd.drawNumber(x+50,y+14,egt,MIDSIZE)
end

local function draw_fuel(x,y,src)
   local fuel = getValue(src)
   lcd.drawPixmap(x+4,y+14,"/SCRIPTS/BMP/gasoline.bmp")
   lcd.drawText(x+38,y+4,"FUEL",0)
   lcd.drawNumber(x+60,y+15,fuel,DBLSIZE)
end

local function draw_battery(x,y,src)
   local batt_v = getValue(src)*10
   lcd.drawPixmap(x+4,y+3,"/SCRIPTS/BMP/battery.bmp")
   lcd.drawNumber(x+60,y+5,batt_v,PREC1)
end

local function draw_pump(x,y,src)
   local pump_v = getValue(src)*10
   lcd.drawText(x+5,y+3,"PUMP",SMLSIZE)
   lcd.drawNumber(x+60,y+2,pump_v,PREC1)
end

local function draw_speed(x,y,src)
   local speed = getValue(src)
   lcd.drawPixmap(x+3,y+3,"/SCRIPTS/BMP/speed.bmp")
   lcd.drawText(x+60,y+4,"SPEED",0)
   lcd.drawNumber(x+90,y+16,speed,DBLSIZE)
end

local function run(event)

   local rpm    = "RPM"
   local egt    = "Tmp1"
   local batt_v = "A3"
   local pump_v = "A4"
   local fuel   = "Fuel"
   local status = "Tmp2"
   local speed  = "ASpd"

   lcd.clear()

   draw_rpm(0,0,rpm)
   lcd.drawLine(0,35,53,35,DOTTED,FORCE)
   draw_egt(0,36,egt)
   lcd.drawLine(54,0,54,63,DOTTED,FORCE)

   draw_speed(55,0,speed)
   lcd.drawLine(55,35,147,35,DOTTED,FORCE)
   draw_status(55,38,status)

   lcd.drawLine(148,0,148,63,DOTTED,FORCE)
   draw_fuel(149,0,fuel)
   lcd.drawLine(149,35,211,35,DOTTED,FORCE)

   draw_battery(149,36,batt_v)
   lcd.drawLine(149,52,211,52,DOTTED,FORCE)
   draw_pump(149,53,pump_v)

   
   --[[

   batt_v = getValue(batt_v)
   lcd.drawText(1,31,"Batt",0)
   lcd.drawNumber(60,31,batt_v*10,PREC1)
   
   pump_v = getValue(pump_v)
   lcd.drawText(1,41,"Pump",0)
   lcd.drawNumber(60,41,pump_v*10,PREC1)

   status = get_status(status)
   lcd.drawText(1,51,status,0)
   --]]
   
end

return {run=run}
