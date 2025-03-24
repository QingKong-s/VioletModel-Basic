#include "pch.h"

LRESULT CPlayPanel::OnEvent(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_PAINT:
    {
        Dui::ELEMPAINTSTRU ps;
        BeginPaint(ps, wParam, lParam);
        m_pDC->FillRectangle(ps.rcfClipInElem, m_pBrush);
        EndPaint(ps);
    }
    return 0;

    case WM_CREATE:
        m_pDC->CreateSolidColorBrush(App->GetColor(GPal::PlayPanelBk), &m_pBrush);
        break;

    case WM_DESTROY:
        SafeRelease(m_pBrush);
        break;
    }
    return __super::OnEvent(uMsg, wParam, lParam);
}
