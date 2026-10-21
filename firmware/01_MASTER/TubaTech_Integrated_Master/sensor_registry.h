#pragma once
#include "api_policy.h"
#include <cmath>
namespace sensors {
struct Sensor {const char *id,*vessel,*type,*method;std::string draftA,draftB,timestamp,note;};
inline Sensor entries[]={
 {"wine_initial_ph","wine_initial","PH-4502C/E201","two_point_ph"},
 {"wine_initial_mq3","wine_initial","MQ-3","mq3_raw_curve"},
 {"wine_initial_level","wine_initial","ultrasonic","distance_scale"},
 {"wine_pc1_ph","wine_PC1","PH-4502C/E201","two_point_ph"},
 {"wine_pc1_mq3","wine_PC1","MQ-3","mq3_raw_curve"},
 {"wine_pc2_ph","wine_PC2","PH-4502C/E201","two_point_ph"},
 {"wine_pc2_mq3","wine_PC2","MQ-3","mq3_raw_curve"},
 {"vinegar_initial_ph","vinegar_initial","PH-4502C/E201","two_point_ph"},
 {"vinegar_initial_mq3","vinegar_initial","MQ-3","mq3_raw_curve"},
 {"vinegar_initial_level","vinegar_initial","ultrasonic","distance_scale"},
 {"vinegar_pc1_ph","vinegar_PC1","PH-4502C/E201","two_point_ph"},
 {"vinegar_pc1_mq3","vinegar_PC1","MQ-3","mq3_raw_curve"},
 {"vinegar_pc1_temperature","vinegar_PC1","DS18B20","temperature_offset"},
 {"vinegar_pc2_ph","vinegar_PC2","PH-4502C/E201","two_point_ph"},
 {"vinegar_pc2_mq3","vinegar_PC2","MQ-3","mq3_raw_curve"},
 {"vinegar_pc2_temperature","vinegar_PC2","DS18B20","temperature_offset"}
};
inline bool coefficient(const std::string &text){
  if(text.empty()||text.size()>32)return false;char *end=nullptr;
  double value=std::strtod(text.c_str(),&end);
  return end==text.c_str()+text.size()&&std::isfinite(value)&&std::fabs(value)<=1000000;
}
inline bool utcTimestamp(const std::string &s){
  if(s.size()<20||s.size()>27||s[4]!='-'||s[7]!='-'||s[10]!='T'||s[13]!=':'||s[16]!=':'||s.back()!='Z')return false;
  auto digits=[&s](size_t at,size_t count){unsigned n=0;for(size_t i=at;i<at+count;++i){if(s[i]<'0'||s[i]>'9')return -1;n=n*10+unsigned(s[i]-'0');}return int(n);};
  int year=digits(0,4),month=digits(5,2),day=digits(8,2),hour=digits(11,2),minute=digits(14,2),second=digits(17,2);
  if(year<1970||month<1||month>12||day<1||hour<0||hour>23||minute<0||minute>59||second<0||second>59)return false;
  const int days[]={31,28,31,30,31,30,31,31,30,31,30,31};
  bool leap=year%4==0&&(year%100!=0||year%400==0);if(day>days[month-1]+(month==2&&leap?1:0))return false;
  if(s.size()>20){if(s[19]!='.'||s.size()<22)return false;for(size_t i=20;i<s.size()-1;++i)if(s[i]<'0'||s[i]>'9')return false;}
  return true;
}
inline bool saveDraft(const local::Fields &f){
  for(auto &s:entries)if(local::get(f,"sensor_id")==s.id){
    auto a=local::get(f,"a"),b=local::get(f,"b"),stamp=local::get(f,"timestamp"),note=local::get(f,"note");
    if(local::get(f,"method")!=s.method||!coefficient(a)||!coefficient(b)||!utcTimestamp(stamp)||note.size()>200)return false;
    s.draftA=a;s.draftB=b;s.timestamp=stamp;s.note=note;return true;
  }return false;
}
inline std::string json(){
  using local::quote;std::string out="{\"sensors\":[";bool first=true;
  for(const auto &s:entries){if(!first)out+=',';first=false;
    out+="{\"sensor_id\":"+quote(s.id)+",\"vessel\":"+quote(s.vessel)+",\"type\":"+quote(s.type)+
      ",\"method\":"+quote(s.method)+",\"node_id\":null,\"gpio\":null,\"bus\":null,\"raw\":null,\"calibrated\":null,\"unit\":null,\"last_updated\":null,\"quality\":\"not_installed\",\"connection\":\"NOT CONNECTED\",\"calibration_status\":\"NOT CALIBRATED\",\"error\":\"Hardware allocation and validated driver pending\",\"calibration\":null,\"draft\":";
    if(s.timestamp.empty())out+="null";else out+="{\"validated\":false,\"persistent\":false,\"a\":"+quote(s.draftA)+",\"b\":"+quote(s.draftB)+",\"timestamp\":"+quote(s.timestamp)+",\"note\":"+quote(s.note)+'}';out+='}';
  }return out+"]}";
}
} // namespace sensors
