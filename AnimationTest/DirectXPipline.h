#pragma once

class DirectXPipline
{
public:
    DirectXPipline();
    virtual ~DirectXPipline();

    virtual bool OnInit(HWND hWnd)=0;
    virtual void OnUpdate()=0;
    virtual void OnRender()=0;
    virtual void OnDestroy()=0;
};