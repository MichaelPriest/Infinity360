#pragma once
#include "HomeCanvas.h"
#include <algorithm>
#include <cmath>
#include <initializer_list>

namespace InfinityIcons {
// Original vector glyphs traced by geometry from the approved Infinity concept.
// One 32-unit coordinate system, uniform optical weight, rounded antialiased caps.
struct Point {float x,y;};
class Painter {
    HomeCanvas& c;int x,y,size;unsigned color;
    int px(float v)const{return int(std::lround(x+v*size/32.f));}
    int py(float v)const{return int(std::lround(y+v*size/32.f));}
public:
    Painter(HomeCanvas& canvas,int left,int top,int s,unsigned ink):c(canvas),x(left),y(top),size(s),color(ink){}
    void line(Point a,Point b){c.Stroke(px(a.x),py(a.y),px(b.x),py(b.y),color,std::max(1,int(std::lround(size*0.065f))));}
    void path(std::initializer_list<Point> p){if(p.size()<2)return;auto it=p.begin();Point last=*it++;for(;it!=p.end();++it){line(last,*it);last=*it;}}
    void curve(Point a,Point b,Point d,Point e){Point last=a;for(int n=1;n<=10;++n){float t=n/10.f,u=1-t;Point next{u*u*u*a.x+3*u*u*t*b.x+3*u*t*t*d.x+t*t*t*e.x,u*u*u*a.y+3*u*u*t*b.y+3*u*t*t*d.y+t*t*t*e.y};line(last,next);last=next;}}
    void arc(float cx,float cy,float r,float start,float end){Point last{cx+r*std::cos(start),cy+r*std::sin(start)};for(int n=1;n<=20;++n){float t=start+(end-start)*n/20.f;Point next{cx+r*std::cos(t),cy+r*std::sin(t)};line(last,next);last=next;}}
    void circle(float cx,float cy,float r){arc(cx,cy,r,0,6.2831853f);}
    void box(float l,float t,float r,float b,float q=2){path({{l+q,t},{r-q,t}});curve({r-q,t},{r,t},{r,t},{r,t+q});path({{r,t+q},{r,b-q}});curve({r,b-q},{r,b},{r,b},{r-q,b});path({{r-q,b},{l+q,b}});curve({l+q,b},{l,b},{l,b},{l,b-q});path({{l,b-q},{l,t+q}});curve({l,t+q},{l,t},{l,t},{l+q,t});}
};
inline void Draw(HomeCanvas& c,int kind,int x,int y,int size,unsigned color){
    if(size<=0)return;
    Painter p(c,x,y,size,color);
    switch(kind){
    case 0: // home
        p.path({{3,15},{16,3},{29,15}});p.path({{7,13},{7,28},{13,28},{13,20},{19,20},{19,28},{25,28},{25,13}});break;
    case 1:case 12: // controller
        p.curve({10,7},{5,5},{3,12},{2.5f,21});p.curve({2.5f,21},{2,29},{8,29},{12,22});p.path({{12,22},{20,22}});p.curve({20,22},{24,29},{30,29},{29.5f,21});p.curve({29.5f,21},{29,12},{27,5},{22,7});p.curve({22,7},{18,9},{14,9},{10,7});p.path({{7,15},{13,15}});p.path({{10,12},{10,18}});p.circle(23,12,0.8f);p.circle(26,15,0.8f);p.circle(23,18,0.8f);p.circle(20,15,0.8f);break;
    case 2: // apps
        p.box(3,3,13,13,1);p.box(19,3,29,13,1);p.box(3,19,13,29,1);p.box(19,19,29,29,1);break;
    case 3:case 16: // television / monitor
        p.box(3,6,29,24,2);if(kind==3){p.path({{10,28},{22,28}});p.path({{16,24},{16,28}});}else{p.path({{16,24},{16,29}});p.path({{10,29},{22,29}});}break;
    case 4:case 17: // shopping bag
        p.path({{10,10},{6,10},{4,28},{28,28},{26,10},{22,10}});p.curve({11,12},{11,2},{21,2},{21,12});break;
    case 5: // download arrow
        p.path({{16,3},{16,22},{9,15}});p.path({{16,22},{23,15}});p.path({{4,25},{4,29},{28,29},{28,25}});break;
    case 6:case 14: // hard drive
        p.path({{4,17},{8,8},{24,8},{28,17}});p.box(3,17,29,28,2);p.circle(24,22.5f,1);p.path({{8,22.5f},{14,22.5f}});break;
    case 7: // Wi-Fi
        p.arc(16,24,17,3.9f,5.52f);p.arc(16,24,11,3.9f,5.52f);p.arc(16,24,5.5f,3.9f,5.52f);p.circle(16,24,0.8f);break;
    case 8: // two profiles
        p.circle(12,9,5);p.curve({3,28},{3,16},{21,16},{21,28});p.arc(23,10,4,-1.5f,1.5f);p.curve({23,19},{29,19},{30,24},{30,28});break;
    case 9:case 18: // trophy
        p.path({{9,4},{23,4},{23,12}});p.curve({23,12},{23,23},{9,23},{9,12});p.path({{9,12},{9,4}});p.curve({9,7},{-1,5},{1,18},{10,17});p.curve({23,7},{33,5},{31,18},{22,17});p.path({{16,21},{16,28},{9,28},{23,28}});break;
    case 10: // gear
        {Point first{},last{};for(int k=0;k<32;++k){float a=6.2831853f*k/32;float r=(k%4==1||k%4==2)?14:11;Point v{16+r*std::cos(a),16+r*std::sin(a)};if(k)p.line(last,v);else first=v;last=v;}p.line(last,first);p.circle(16,16,5);}break;
    case 11: // power
        p.arc(16,17,12,-0.95f,4.1f);p.path({{16,2},{16,16}});break;
    case 13: // optical disc
        p.circle(16,16,13);p.circle(16,16,3);break;
    case 15: // system link
        p.box(12,2,20,10,1);p.box(2,22,10,30,1);p.box(22,22,30,30,1);p.path({{16,10},{16,16},{6,16},{6,22}});p.path({{16,16},{26,16},{26,22}});break;
    case 19: // speech bubble
        p.path({{10,23},{6,23}});p.curve({6,23},{3,23},{3,21},{3,19});p.path({{3,19},{3,8}});p.curve({3,8},{3,4},{4,4},{7,4});p.path({{7,4},{25,4}});p.curve({25,4},{29,4},{29,5},{29,8});p.path({{29,8},{29,19}});p.curve({29,19},{29,22},{28,23},{25,23});p.path({{25,23},{17,23},{12,29},{10,23}});break;
    case 20: // clock
        p.circle(16,16,13);p.path({{16,8},{16,16},{24,16}});break;
    case 21: // star
        p.path({{16,3},{20,12},{30,13},{22,20},{25,29},{16,24},{7,29},{10,20},{2,13},{12,12},{16,3}});break;
    case 22: // notification bell
        p.curve({9,12},{9,3},{23,3},{23,12});p.curve({9,12},{9,19},{8,22},{5,25});p.path({{5,25},{27,25}});p.curve({27,25},{24,22},{23,19},{23,12});p.arc(16,26,4,0,3.14159f);break;
    case 23: // library books
        p.box(4,4,10,28,1);p.box(13,4,19,28,1);p.path({{22,5},{27,4},{31,27},{26,28},{22,5}});break;
    case 24: // stacked Guide layers
        p.path({{2,10},{16,3},{30,10},{16,17},{2,10}});p.path({{2,16},{16,23},{30,16}});p.path({{2,22},{16,29},{30,22}});break;
    case 25: // cloud
        p.curve({8,25},{-1,25},{-1,14},{8,14});p.curve({8,14},{8,3},{24,3},{24,14});p.curve({24,14},{34,13},{34,25},{25,25});p.path({{25,25},{8,25}});break;
    case 26: // CPU chip
        p.box(7,7,25,25,2);for(int n=10;n<=22;n+=6){p.path({{n,2},{n,7}});p.path({{n,25},{n,30}});p.path({{2,n},{7,n}});p.path({{25,n},{30,n}});}break;
    case 27: // single avatar
        p.circle(16,9,5);p.curve({5,28},{5,15},{27,15},{27,28});p.path({{27,28},{5,28}});break;
    default:break;
    }
}
}
