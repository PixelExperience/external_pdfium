// Copyright 2014 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "xfa/include/fxgraphics/fx_graphics.h"

#include <memory>

#include "xfa/fxgraphics/fx_path_generator.h"
#include "xfa/fxgraphics/pre.h"

CFX_Graphics::CFX_Graphics()
    : m_renderDevice(nullptr), m_aggGraphics(nullptr) {}

FX_ERR CFX_Graphics::Create(CFX_RenderDevice* renderDevice,
                            FX_BOOL isAntialiasing) {
  if (!renderDevice)
    return FX_ERR_Parameter_Invalid;
  if (m_type != FX_CONTEXT_None)
    return FX_ERR_Property_Invalid;

  m_type = FX_CONTEXT_Device;
  m_info.isAntialiasing = isAntialiasing;
  m_renderDevice = renderDevice;
  if (m_renderDevice->GetDeviceCaps(FXDC_RENDER_CAPS) & FXRC_SOFT_CLIP)
    return FX_ERR_Succeeded;
  return FX_ERR_Indefinite;
}

FX_ERR CFX_Graphics::Create(int32_t width,
                            int32_t height,
                            FXDIB_Format format,
                            FX_BOOL isNative,
                            FX_BOOL isAntialiasing) {
  if (m_type != FX_CONTEXT_None)
    return FX_ERR_Property_Invalid;

  m_type = FX_CONTEXT_Device;
  m_info.isAntialiasing = isAntialiasing;
  m_aggGraphics = new CAGG_Graphics;
  return m_aggGraphics->Create(this, width, height, format);
}

CFX_Graphics::~CFX_Graphics() {
  delete m_aggGraphics;
}

FX_ERR CFX_Graphics::GetDeviceCap(const int32_t capID, FX_DeviceCap& capVal) {
  if (m_type == FX_CONTEXT_Device && m_renderDevice) {
    capVal = m_renderDevice->GetDeviceCaps(capID);
    return FX_ERR_Succeeded;
  }
  return FX_ERR_Property_Invalid;
}

FX_ERR CFX_Graphics::IsPrinterDevice(FX_BOOL& isPrinter) {
  if (m_type == FX_CONTEXT_Device && m_renderDevice) {
    isPrinter = m_renderDevice->GetDeviceClass() == FXDC_PRINTER;
    return FX_ERR_Succeeded;
  }
  return FX_ERR_Property_Invalid;
}

FX_ERR CFX_Graphics::EnableAntialiasing(FX_BOOL isAntialiasing) {
  if (m_type == FX_CONTEXT_Device && m_renderDevice) {
    m_info.isAntialiasing = isAntialiasing;
    return FX_ERR_Succeeded;
  }
  return FX_ERR_Property_Invalid;
}

FX_ERR CFX_Graphics::SaveGraphState() {
  if (m_type == FX_CONTEXT_Device && m_renderDevice) {
    m_renderDevice->SaveState();
    m_infoStack.Add(new TInfo(m_info));
    return FX_ERR_Succeeded;
  }
  return FX_ERR_Property_Invalid;
}

FX_ERR CFX_Graphics::RestoreGraphState() {
  if (m_type == FX_CONTEXT_Device && m_renderDevice) {
    m_renderDevice->RestoreState();
    int32_t size = m_infoStack.GetSize();
    if (size <= 0) {
      return FX_ERR_Intermediate_Value_Invalid;
    }
    int32_t topIndex = size - 1;
    std::unique_ptr<TInfo> info(
        reinterpret_cast<TInfo*>(m_infoStack.GetAt(topIndex)));
    if (!info)
      return FX_ERR_Intermediate_Value_Invalid;
    m_info = *info;
    m_infoStack.RemoveAt(topIndex);
    return FX_ERR_Succeeded;
  }
  return FX_ERR_Property_Invalid;
}

FX_ERR CFX_Graphics::GetLineCap(CFX_GraphStateData::LineCap& lineCap) const {
  if (m_type == FX_CONTEXT_Device && m_renderDevice) {
    lineCap = m_info.graphState.m_LineCap;
    return FX_ERR_Succeeded;
  }
  return FX_ERR_Property_Invalid;
}

FX_ERR CFX_Graphics::SetLineCap(CFX_GraphStateData::LineCap lineCap) {
  if (m_type == FX_CONTEXT_Device && m_renderDevice) {
    m_info.graphState.m_LineCap = lineCap;
    return FX_ERR_Succeeded;
  }
  return FX_ERR_Property_Invalid;
}

FX_ERR CFX_Graphics::GetDashCount(int32_t& dashCount) const {
  if (m_type == FX_CONTEXT_Device && m_renderDevice) {
    dashCount = m_info.graphState.m_DashCount;
    return FX_ERR_Succeeded;
  }
  return FX_ERR_Property_Invalid;
}

FX_ERR CFX_Graphics::GetLineDash(FX_FLOAT& dashPhase,
                                 FX_FLOAT* dashArray) const {
  if (!dashArray)
    return FX_ERR_Parameter_Invalid;
  if (m_type == FX_CONTEXT_Device && m_renderDevice) {
    dashPhase = m_info.graphState.m_DashPhase;
    FXSYS_memcpy(dashArray, m_info.graphState.m_DashArray,
                 m_info.graphState.m_DashCount * sizeof(FX_FLOAT));
    return FX_ERR_Succeeded;
  }
  return FX_ERR_Property_Invalid;
}

FX_ERR CFX_Graphics::SetLineDash(FX_FLOAT dashPhase,
                                 FX_FLOAT* dashArray,
                                 int32_t dashCount) {
  if (dashCount > 0 && !dashArray)
    return FX_ERR_Parameter_Invalid;

  dashCount = dashCount < 0 ? 0 : dashCount;
  if (m_type == FX_CONTEXT_Device && m_renderDevice) {
    FX_FLOAT scale = 1.0;
    if (m_info.isActOnDash) {
      scale = m_info.graphState.m_LineWidth;
    }
    m_info.graphState.m_DashPhase = dashPhase;
    m_info.graphState.SetDashCount(dashCount);
    for (int32_t i = 0; i < dashCount; i++) {
      m_info.graphState.m_DashArray[i] = dashArray[i] * scale;
    }
    return FX_ERR_Succeeded;
  }
  return FX_ERR_Property_Invalid;
}

FX_ERR CFX_Graphics::SetLineDash(FX_DashStyle dashStyle) {
  if (m_type == FX_CONTEXT_Device && m_renderDevice)
    return RenderDeviceSetLineDash(dashStyle);
  return FX_ERR_Property_Invalid;
}

FX_ERR CFX_Graphics::GetLineJoin(CFX_GraphStateData::LineJoin& lineJoin) const {
  if (m_type == FX_CONTEXT_Device && m_renderDevice) {
    lineJoin = m_info.graphState.m_LineJoin;
    return FX_ERR_Succeeded;
  }
  return FX_ERR_Property_Invalid;
}

FX_ERR CFX_Graphics::SetLineJoin(CFX_GraphStateData::LineJoin lineJoin) {
  if (m_type == FX_CONTEXT_Device && m_renderDevice) {
    m_info.graphState.m_LineJoin = lineJoin;
    return FX_ERR_Succeeded;
  }
  return FX_ERR_Property_Invalid;
}

FX_ERR CFX_Graphics::GetMiterLimit(FX_FLOAT& miterLimit) const {
  if (m_type == FX_CONTEXT_Device && m_renderDevice) {
    miterLimit = m_info.graphState.m_MiterLimit;
    return FX_ERR_Succeeded;
  }
  return FX_ERR_Property_Invalid;
}

FX_ERR CFX_Graphics::SetMiterLimit(FX_FLOAT miterLimit) {
  if (m_type == FX_CONTEXT_Device && m_renderDevice) {
    m_info.graphState.m_MiterLimit = miterLimit;
    return FX_ERR_Succeeded;
  }
  return FX_ERR_Property_Invalid;
}

FX_ERR CFX_Graphics::GetLineWidth(FX_FLOAT& lineWidth) const {
  if (m_type == FX_CONTEXT_Device && m_renderDevice) {
    lineWidth = m_info.graphState.m_LineWidth;
    return FX_ERR_Succeeded;
  }
  return FX_ERR_Property_Invalid;
}

FX_ERR CFX_Graphics::SetLineWidth(FX_FLOAT lineWidth, FX_BOOL isActOnDash) {
  if (m_type == FX_CONTEXT_Device && m_renderDevice) {
    m_info.graphState.m_LineWidth = lineWidth;
    m_info.isActOnDash = isActOnDash;
    return FX_ERR_Succeeded;
  }
  return FX_ERR_Property_Invalid;
}

FX_ERR CFX_Graphics::GetStrokeAlignment(
    FX_StrokeAlignment& strokeAlignment) const {
  if (m_type == FX_CONTEXT_Device && m_renderDevice) {
    strokeAlignment = m_info.strokeAlignment;
    return FX_ERR_Succeeded;
  }
  return FX_ERR_Property_Invalid;
}

FX_ERR CFX_Graphics::SetStrokeAlignment(FX_StrokeAlignment strokeAlignment) {
  if (m_type == FX_CONTEXT_Device && m_renderDevice) {
    m_info.strokeAlignment = strokeAlignment;
    return FX_ERR_Succeeded;
  }
  return FX_ERR_Property_Invalid;
}

FX_ERR CFX_Graphics::SetStrokeColor(CFX_Color* color) {
  if (!color)
    return FX_ERR_Parameter_Invalid;
  if (m_type == FX_CONTEXT_Device && m_renderDevice) {
    m_info.strokeColor = color;
    return FX_ERR_Succeeded;
  }
  return FX_ERR_Property_Invalid;
}

FX_ERR CFX_Graphics::SetFillColor(CFX_Color* color) {
  if (!color)
    return FX_ERR_Parameter_Invalid;
  if (m_type == FX_CONTEXT_Device && m_renderDevice) {
    m_info.fillColor = color;
    return FX_ERR_Succeeded;
  }
  return FX_ERR_Property_Invalid;
}

FX_ERR CFX_Graphics::StrokePath(CFX_Path* path, CFX_Matrix* matrix) {
  if (!path)
    return FX_ERR_Parameter_Invalid;
  if (m_type == FX_CONTEXT_Device && m_renderDevice)
    return RenderDeviceStrokePath(path, matrix);
  return FX_ERR_Property_Invalid;
}

FX_ERR CFX_Graphics::FillPath(CFX_Path* path,
                              FX_FillMode fillMode,
                              CFX_Matrix* matrix) {
  if (!path)
    return FX_ERR_Parameter_Invalid;
  if (m_type == FX_CONTEXT_Device && m_renderDevice)
    return RenderDeviceFillPath(path, fillMode, matrix);
  return FX_ERR_Property_Invalid;
}

FX_ERR CFX_Graphics::ClipPath(CFX_Path* path,
                              FX_FillMode fillMode,
                              CFX_Matrix* matrix) {
  if (!path)
    return FX_ERR_Parameter_Invalid;
  if (m_type == FX_CONTEXT_Device && m_renderDevice) {
    FX_BOOL result = m_renderDevice->SetClip_PathFill(
        path->GetPathData(), (CFX_Matrix*)matrix, fillMode);
    if (!result)
      return FX_ERR_Indefinite;
    return FX_ERR_Succeeded;
  }
  return FX_ERR_Property_Invalid;
}

FX_ERR CFX_Graphics::DrawImage(CFX_DIBSource* source,
                               const CFX_PointF& point,
                               CFX_Matrix* matrix) {
  if (!source)
    return FX_ERR_Parameter_Invalid;
  if (m_type == FX_CONTEXT_Device && m_renderDevice)
    return RenderDeviceDrawImage(source, point, matrix);
  return FX_ERR_Property_Invalid;
}

FX_ERR CFX_Graphics::StretchImage(CFX_DIBSource* source,
                                  const CFX_RectF& rect,
                                  CFX_Matrix* matrix) {
  if (!source)
    return FX_ERR_Parameter_Invalid;
  if (m_type == FX_CONTEXT_Device && m_renderDevice)
    return RenderDeviceStretchImage(source, rect, matrix);
  return FX_ERR_Property_Invalid;
}

FX_ERR CFX_Graphics::ConcatMatrix(const CFX_Matrix* matrix) {
  if (!matrix)
    return FX_ERR_Parameter_Invalid;
  if (m_type == FX_CONTEXT_Device && m_renderDevice) {
    m_info.CTM.Concat(*matrix);
    return FX_ERR_Succeeded;
  }
  return FX_ERR_Property_Invalid;
}

CFX_Matrix* CFX_Graphics::GetMatrix() {
  if (m_type == FX_CONTEXT_Device && m_renderDevice)
    return &m_info.CTM;
  return nullptr;
}

FX_ERR CFX_Graphics::GetClipRect(CFX_RectF& rect) const {
  if (m_type == FX_CONTEXT_Device && m_renderDevice) {
    FX_RECT r = m_renderDevice->GetClipBox();
    rect.left = (FX_FLOAT)r.left;
    rect.top = (FX_FLOAT)r.top;
    rect.width = (FX_FLOAT)r.Width();
    rect.height = (FX_FLOAT)r.Height();
    return FX_ERR_Succeeded;
  }
  return FX_ERR_Property_Invalid;
}

FX_ERR CFX_Graphics::SetClipRect(const CFX_RectF& rect) {
  if (m_type == FX_CONTEXT_Device && m_renderDevice) {
    if (!m_renderDevice->SetClip_Rect(
            FX_RECT(FXSYS_round(rect.left), FXSYS_round(rect.top),
                    FXSYS_round(rect.right()), FXSYS_round(rect.bottom())))) {
      return FX_ERR_Method_Not_Supported;
    }
    return FX_ERR_Succeeded;
  }
  return FX_ERR_Property_Invalid;
}

FX_ERR CFX_Graphics::ClearClip() {
  if (m_type == FX_CONTEXT_Device && m_renderDevice)
    return FX_ERR_Succeeded;
  return FX_ERR_Property_Invalid;
}

FX_ERR CFX_Graphics::SetFont(CFX_Font* font) {
  if (!font)
    return FX_ERR_Parameter_Invalid;
  if (m_type == FX_CONTEXT_Device && m_renderDevice) {
    m_info.font = font;
    return FX_ERR_Succeeded;
  }
  return FX_ERR_Property_Invalid;
}

FX_ERR CFX_Graphics::SetFontSize(const FX_FLOAT size) {
  if (m_type == FX_CONTEXT_Device && m_renderDevice) {
    m_info.fontSize = size <= 0 ? 1.0f : size;
    return FX_ERR_Succeeded;
  }
  return FX_ERR_Property_Invalid;
}

FX_ERR CFX_Graphics::SetFontHScale(const FX_FLOAT scale) {
  if (m_type == FX_CONTEXT_Device && m_renderDevice) {
    m_info.fontHScale = scale <= 0 ? 1.0f : scale;
    return FX_ERR_Succeeded;
  }
  return FX_ERR_Property_Invalid;
}

FX_ERR CFX_Graphics::SetCharSpacing(const FX_FLOAT spacing) {
  if (m_type == FX_CONTEXT_Device && m_renderDevice) {
    m_info.fontSpacing = spacing < 0 ? 0 : spacing;
    return FX_ERR_Succeeded;
  }
  return FX_ERR_Property_Invalid;
}

FX_ERR CFX_Graphics::SetTextDrawingMode(const int32_t mode) {
  if (m_type == FX_CONTEXT_Device && m_renderDevice)
    return FX_ERR_Succeeded;
  return FX_ERR_Property_Invalid;
}

FX_ERR CFX_Graphics::ShowText(const CFX_PointF& point,
                              const CFX_WideString& text,
                              CFX_Matrix* matrix) {
  if (m_type == FX_CONTEXT_Device && m_renderDevice)
    return RenderDeviceShowText(point, text, matrix);
  return FX_ERR_Property_Invalid;
}

FX_ERR CFX_Graphics::CalcTextRect(CFX_RectF& rect,
                                  const CFX_WideString& text,
                                  FX_BOOL isMultiline,
                                  CFX_Matrix* matrix) {
  if (m_type == FX_CONTEXT_Device && m_renderDevice) {
    int32_t length = text.GetLength();
    FX_DWORD* charCodes = FX_Alloc(FX_DWORD, length);
    FXTEXT_CHARPOS* charPos = FX_Alloc(FXTEXT_CHARPOS, length);
    CalcTextInfo(text, charCodes, charPos, rect);
    FX_Free(charPos);
    FX_Free(charCodes);
    return FX_ERR_Succeeded;
  }
  return FX_ERR_Property_Invalid;
}

FX_ERR CFX_Graphics::Transfer(CFX_Graphics* graphics,
                              const CFX_Matrix* matrix) {
  if (!graphics || !graphics->m_renderDevice)
    return FX_ERR_Parameter_Invalid;
  CFX_Matrix m;
  m.Set(m_info.CTM.a, m_info.CTM.b, m_info.CTM.c, m_info.CTM.d, m_info.CTM.e,
        m_info.CTM.f);
  if (matrix) {
    m.Concat(*matrix);
  }
  if (m_type == FX_CONTEXT_Device && m_renderDevice) {
    CFX_DIBitmap* bitmap = graphics->m_renderDevice->GetBitmap();
    FX_BOOL result = m_renderDevice->SetDIBits(bitmap, 0, 0);
    if (!result)
      return FX_ERR_Method_Not_Supported;
    return FX_ERR_Succeeded;
  }
  return FX_ERR_Property_Invalid;
}

FX_ERR CFX_Graphics::Transfer(CFX_Graphics* graphics,
                              FX_FLOAT srcLeft,
                              FX_FLOAT srcTop,
                              const CFX_RectF& dstRect,
                              const CFX_Matrix* matrix) {
  if (!graphics || !graphics->m_renderDevice)
    return FX_ERR_Parameter_Invalid;
  CFX_Matrix m;
  m.Set(m_info.CTM.a, m_info.CTM.b, m_info.CTM.c, m_info.CTM.d, m_info.CTM.e,
        m_info.CTM.f);
  if (matrix) {
    m.Concat(*matrix);
  }
  if (m_type == FX_CONTEXT_Device && m_renderDevice) {
    CFX_DIBitmap bmp;
    FX_BOOL result =
        bmp.Create((int32_t)dstRect.width, (int32_t)dstRect.height,
                   graphics->m_renderDevice->GetBitmap()->GetFormat());
    if (!result)
      return FX_ERR_Intermediate_Value_Invalid;
    result = graphics->m_renderDevice->GetDIBits(&bmp, (int32_t)srcLeft,
                                                 (int32_t)srcTop);
    if (!result)
      return FX_ERR_Method_Not_Supported;
    result = m_renderDevice->SetDIBits(&bmp, (int32_t)dstRect.left,
                                       (int32_t)dstRect.top);
    if (!result)
      return FX_ERR_Method_Not_Supported;
    return FX_ERR_Succeeded;
  }
  return FX_ERR_Property_Invalid;
}

CFX_RenderDevice* CFX_Graphics::GetRenderDevice() {
  return m_renderDevice;
}

FX_ERR CFX_Graphics::InverseRect(const CFX_RectF& rect) {
  if (!m_renderDevice)
    return FX_ERR_Property_Invalid;
  CFX_DIBitmap* bitmap = m_renderDevice->GetBitmap();
  if (!bitmap)
    return FX_ERR_Property_Invalid;
  CFX_RectF temp(rect);
  m_info.CTM.TransformRect(temp);
  CFX_RectF r;
  r.Set(0, 0, (FX_FLOAT)bitmap->GetWidth(), (FX_FLOAT)bitmap->GetWidth());
  r.Intersect(temp);
  if (r.IsEmpty()) {
    return FX_ERR_Parameter_Invalid;
  }
  FX_ARGB* pBuf =
      (FX_ARGB*)(bitmap->GetBuffer() + int32_t(r.top) * bitmap->GetPitch());
  int32_t bottom = (int32_t)r.bottom();
  int32_t right = (int32_t)r.right();
  for (int32_t i = (int32_t)r.top; i < bottom; i++) {
    FX_ARGB* pLine = pBuf + (int32_t)r.left;
    for (int32_t j = (int32_t)r.left; j < right; j++) {
      FX_ARGB c = *pLine;
      *pLine++ = (c & 0xFF000000) | (0xFFFFFF - (c & 0x00FFFFFF));
    }
    pBuf = (FX_ARGB*)((uint8_t*)pBuf + bitmap->GetPitch());
  }
  return FX_ERR_Succeeded;
}

FX_ERR CFX_Graphics::XorDIBitmap(const CFX_DIBitmap* srcBitmap,
                                 const CFX_RectF& rect) {
  if (!m_renderDevice)
    return FX_ERR_Property_Invalid;
  CFX_DIBitmap* dst = m_renderDevice->GetBitmap();
  if (!dst)
    return FX_ERR_Property_Invalid;
  CFX_RectF temp(rect);
  m_info.CTM.TransformRect(temp);
  CFX_RectF r;
  r.Set(0, 0, (FX_FLOAT)dst->GetWidth(), (FX_FLOAT)dst->GetWidth());
  r.Intersect(temp);
  if (r.IsEmpty()) {
    return FX_ERR_Parameter_Invalid;
  }
  FX_ARGB* pSrcBuf = (FX_ARGB*)(srcBitmap->GetBuffer() +
                                int32_t(r.top) * srcBitmap->GetPitch());
  FX_ARGB* pDstBuf =
      (FX_ARGB*)(dst->GetBuffer() + int32_t(r.top) * dst->GetPitch());
  int32_t bottom = (int32_t)r.bottom();
  int32_t right = (int32_t)r.right();
  for (int32_t i = (int32_t)r.top; i < bottom; i++) {
    FX_ARGB* pSrcLine = pSrcBuf + (int32_t)r.left;
    FX_ARGB* pDstLine = pDstBuf + (int32_t)r.left;
    for (int32_t j = (int32_t)r.left; j < right; j++) {
      FX_ARGB c = *pDstLine;
      *pDstLine++ =
          ArgbEncode(FXARGB_A(c), (c & 0xFFFFFF) ^ (*pSrcLine & 0xFFFFFF));
      pSrcLine++;
    }
    pSrcBuf = (FX_ARGB*)((uint8_t*)pSrcBuf + srcBitmap->GetPitch());
    pDstBuf = (FX_ARGB*)((uint8_t*)pDstBuf + dst->GetPitch());
  }
  return FX_ERR_Succeeded;
}

FX_ERR CFX_Graphics::EqvDIBitmap(const CFX_DIBitmap* srcBitmap,
                                 const CFX_RectF& rect) {
  if (!m_renderDevice)
    return FX_ERR_Property_Invalid;
  CFX_DIBitmap* dst = m_renderDevice->GetBitmap();
  if (!dst)
    return FX_ERR_Property_Invalid;
  CFX_RectF temp(rect);
  m_info.CTM.TransformRect(temp);
  CFX_RectF r;
  r.Set(0, 0, (FX_FLOAT)dst->GetWidth(), (FX_FLOAT)dst->GetWidth());
  r.Intersect(temp);
  if (r.IsEmpty()) {
    return FX_ERR_Parameter_Invalid;
  }
  FX_ARGB* pSrcBuf = (FX_ARGB*)(srcBitmap->GetBuffer() +
                                int32_t(r.top) * srcBitmap->GetPitch());
  FX_ARGB* pDstBuf =
      (FX_ARGB*)(dst->GetBuffer() + int32_t(r.top) * dst->GetPitch());
  int32_t bottom = (int32_t)r.bottom();
  int32_t right = (int32_t)r.right();
  for (int32_t i = (int32_t)r.top; i < bottom; i++) {
    FX_ARGB* pSrcLine = pSrcBuf + (int32_t)r.left;
    FX_ARGB* pDstLine = pDstBuf + (int32_t)r.left;
    for (int32_t j = (int32_t)r.left; j < right; j++) {
      FX_ARGB c = *pDstLine;
      *pDstLine++ =
          ArgbEncode(FXARGB_A(c), ~((c & 0xFFFFFF) ^ (*pSrcLine & 0xFFFFFF)));
      pSrcLine++;
    }
    pSrcBuf = (FX_ARGB*)((uint8_t*)pSrcBuf + srcBitmap->GetPitch());
    pDstBuf = (FX_ARGB*)((uint8_t*)pDstBuf + dst->GetPitch());
  }
  return FX_ERR_Succeeded;
}

FX_ERR CFX_Graphics::RenderDeviceSetLineDash(FX_DashStyle dashStyle) {
  switch (dashStyle) {
    case FX_DASHSTYLE_Solid: {
      m_info.graphState.SetDashCount(0);
      return FX_ERR_Succeeded;
    }
    case FX_DASHSTYLE_Dash: {
      FX_FLOAT dashArray[] = {3, 1};
      SetLineDash(0, dashArray, 2);
      return FX_ERR_Succeeded;
    }
    case FX_DASHSTYLE_Dot: {
      FX_FLOAT dashArray[] = {1, 1};
      SetLineDash(0, dashArray, 2);
      return FX_ERR_Succeeded;
    }
    case FX_DASHSTYLE_DashDot: {
      FX_FLOAT dashArray[] = {3, 1, 1, 1};
      SetLineDash(0, dashArray, 4);
      return FX_ERR_Succeeded;
    }
    case FX_DASHSTYLE_DashDotDot: {
      FX_FLOAT dashArray[] = {4, 1, 2, 1, 2, 1};
      SetLineDash(0, dashArray, 6);
      return FX_ERR_Succeeded;
    }
    default:
      return FX_ERR_Parameter_Invalid;
  }
}

FX_ERR CFX_Graphics::RenderDeviceStrokePath(CFX_Path* path,
                                            CFX_Matrix* matrix) {
  if (!m_info.strokeColor)
    return FX_ERR_Property_Invalid;
  CFX_Matrix m;
  m.Set(m_info.CTM.a, m_info.CTM.b, m_info.CTM.c, m_info.CTM.d, m_info.CTM.e,
        m_info.CTM.f);
  if (matrix) {
    m.Concat(*matrix);
  }
  switch (m_info.strokeColor->m_type) {
    case FX_COLOR_Solid: {
      FX_BOOL result = m_renderDevice->DrawPath(
          path->GetPathData(), (CFX_Matrix*)&m, &m_info.graphState, 0x0,
          m_info.strokeColor->m_info.argb, 0);
      if (!result)
        return FX_ERR_Indefinite;
      return FX_ERR_Succeeded;
    }
    case FX_COLOR_Pattern:
      return StrokePathWithPattern(path, &m);
    case FX_COLOR_Shading:
      return StrokePathWithShading(path, &m);
    default:
      return FX_ERR_Property_Invalid;
  }
}

FX_ERR CFX_Graphics::RenderDeviceFillPath(CFX_Path* path,
                                          FX_FillMode fillMode,
                                          CFX_Matrix* matrix) {
  if (!m_info.fillColor)
    return FX_ERR_Property_Invalid;
  CFX_Matrix m;
  m.Set(m_info.CTM.a, m_info.CTM.b, m_info.CTM.c, m_info.CTM.d, m_info.CTM.e,
        m_info.CTM.f);
  if (matrix) {
    m.Concat(*matrix);
  }
  switch (m_info.fillColor->m_type) {
    case FX_COLOR_Solid: {
      FX_BOOL result = m_renderDevice->DrawPath(
          path->GetPathData(), (CFX_Matrix*)&m, &m_info.graphState,
          m_info.fillColor->m_info.argb, 0x0, fillMode);
      if (!result)
        return FX_ERR_Indefinite;
      return FX_ERR_Succeeded;
    }
    case FX_COLOR_Pattern:
      return FillPathWithPattern(path, fillMode, &m);
    case FX_COLOR_Shading:
      return FillPathWithShading(path, fillMode, &m);
    default:
      return FX_ERR_Property_Invalid;
  }
}

FX_ERR CFX_Graphics::RenderDeviceDrawImage(CFX_DIBSource* source,
                                           const CFX_PointF& point,
                                           CFX_Matrix* matrix) {
  CFX_Matrix m1;
  m1.Set(m_info.CTM.a, m_info.CTM.b, m_info.CTM.c, m_info.CTM.d, m_info.CTM.e,
         m_info.CTM.f);
  if (matrix) {
    m1.Concat(*matrix);
  }
  CFX_Matrix m2;
  m2.Set((FX_FLOAT)source->GetWidth(), 0.0, 0.0, (FX_FLOAT)source->GetHeight(),
         point.x, point.y);
  m2.Concat(m1);
  int32_t left, top;
  std::unique_ptr<CFX_DIBitmap> bmp1(source->FlipImage(FALSE, TRUE));
  std::unique_ptr<CFX_DIBitmap> bmp2(
      bmp1->TransformTo((CFX_Matrix*)&m2, left, top));
  CFX_RectF r;
  GetClipRect(r);
  CFX_DIBitmap* bitmap = m_renderDevice->GetBitmap();
  CFX_DIBitmap bmp;
  if (bmp.Create(bitmap->GetWidth(), bitmap->GetHeight(), FXDIB_Argb) &&
      m_renderDevice->GetDIBits(&bmp, 0, 0) &&
      bmp.TransferBitmap(FXSYS_round(r.left), FXSYS_round(r.top),
                         FXSYS_round(r.Width()), FXSYS_round(r.Height()),
                         bmp2.get(), FXSYS_round(r.left - left),
                         FXSYS_round(r.top - top)) &&
      m_renderDevice->SetDIBits(&bmp, 0, 0)) {
    return FX_ERR_Succeeded;
  }
  return FX_ERR_Indefinite;
}

FX_ERR CFX_Graphics::RenderDeviceStretchImage(CFX_DIBSource* source,
                                              const CFX_RectF& rect,
                                              CFX_Matrix* matrix) {
  CFX_Matrix m1;
  m1.Set(m_info.CTM.a, m_info.CTM.b, m_info.CTM.c, m_info.CTM.d, m_info.CTM.e,
         m_info.CTM.f);
  if (matrix) {
    m1.Concat(*matrix);
  }
  std::unique_ptr<CFX_DIBitmap> bmp1(
      source->StretchTo((int32_t)rect.Width(), (int32_t)rect.Height()));
  CFX_Matrix m2;
  m2.Set(rect.Width(), 0.0, 0.0, rect.Height(), rect.left, rect.top);
  m2.Concat(m1);
  int32_t left, top;
  std::unique_ptr<CFX_DIBitmap> bmp2(bmp1->FlipImage(FALSE, TRUE));
  std::unique_ptr<CFX_DIBitmap> bmp3(
      bmp2->TransformTo((CFX_Matrix*)&m2, left, top));
  CFX_RectF r;
  GetClipRect(r);
  CFX_DIBitmap* bitmap = m_renderDevice->GetBitmap();
  if (bitmap->CompositeBitmap(FXSYS_round(r.left), FXSYS_round(r.top),
                              FXSYS_round(r.Width()), FXSYS_round(r.Height()),
                              bmp3.get(), FXSYS_round(r.left - left),
                              FXSYS_round(r.top - top))) {
    return FX_ERR_Succeeded;
  }
  return FX_ERR_Indefinite;
}

FX_ERR CFX_Graphics::RenderDeviceShowText(const CFX_PointF& point,
                                          const CFX_WideString& text,
                                          CFX_Matrix* matrix) {
  int32_t length = text.GetLength();
  FX_DWORD* charCodes = FX_Alloc(FX_DWORD, length);
  FXTEXT_CHARPOS* charPos = FX_Alloc(FXTEXT_CHARPOS, length);
  CFX_RectF rect;
  rect.Set(point.x, point.y, 0, 0);
  CalcTextInfo(text, charCodes, charPos, rect);
  CFX_Matrix m;
  m.Set(m_info.CTM.a, m_info.CTM.b, m_info.CTM.c, m_info.CTM.d, m_info.CTM.e,
        m_info.CTM.f);
  m.Translate(0, m_info.fontSize * m_info.fontHScale);
  if (matrix) {
    m.Concat(*matrix);
  }
  FX_BOOL result = m_renderDevice->DrawNormalText(
      length, charPos, m_info.font, CFX_GEModule::Get()->GetFontCache(),
      -m_info.fontSize * m_info.fontHScale, (CFX_Matrix*)&m,
      m_info.fillColor->m_info.argb, FXTEXT_CLEARTYPE);
  if (!result)
    return FX_ERR_Indefinite;
  FX_Free(charPos);
  FX_Free(charCodes);
  return FX_ERR_Succeeded;
}

FX_ERR CFX_Graphics::StrokePathWithPattern(CFX_Path* path, CFX_Matrix* matrix) {
  return FX_ERR_Method_Not_Supported;
}

FX_ERR CFX_Graphics::StrokePathWithShading(CFX_Path* path, CFX_Matrix* matrix) {
  return FX_ERR_Method_Not_Supported;
}

FX_ERR CFX_Graphics::FillPathWithPattern(CFX_Path* path,
                                         FX_FillMode fillMode,
                                         CFX_Matrix* matrix) {
  CFX_Pattern* pattern = m_info.fillColor->m_info.pattern;
  CFX_DIBitmap* bitmap = m_renderDevice->GetBitmap();
  int32_t width = bitmap->GetWidth();
  int32_t height = bitmap->GetHeight();
  CFX_DIBitmap bmp;
  bmp.Create(width, height, FXDIB_Argb);
  m_renderDevice->GetDIBits(&bmp, 0, 0);
  switch (pattern->m_type) {
    case FX_PATTERN_Bitmap: {
      int32_t xStep = FXSYS_round(pattern->m_bitmapInfo.x1Step);
      int32_t yStep = FXSYS_round(pattern->m_bitmapInfo.y1Step);
      int32_t xCount = width / xStep + 1;
      int32_t yCount = height / yStep + 1;
      for (int32_t i = 0; i <= yCount; i++) {
        for (int32_t j = 0; j <= xCount; j++) {
          bmp.TransferBitmap(j * xStep, i * yStep, xStep, yStep,
                             pattern->m_bitmapInfo.bitmap, 0, 0);
        }
      }
      break;
    }
    case FX_PATTERN_Hatch: {
      FX_HatchStyle hatchStyle =
          m_info.fillColor->m_info.pattern->m_hatchInfo.hatchStyle;
      if (hatchStyle < FX_HATCHSTYLE_Horizontal ||
          hatchStyle > FX_HATCHSTYLE_SolidDiamond) {
        return FX_ERR_Intermediate_Value_Invalid;
      }
      const FX_HATCHDATA& data = hatchBitmapData[hatchStyle];
      CFX_DIBitmap mask;
      mask.Create(data.width, data.height, FXDIB_1bppMask);
      FXSYS_memcpy(mask.GetBuffer(), data.maskBits,
                   mask.GetPitch() * data.height);
      CFX_FloatRect rectf = path->GetPathData()->GetBoundingBox();
      if (matrix) {
        rectf.Transform((const CFX_Matrix*)matrix);
      }
      FX_RECT rect(FXSYS_round(rectf.left), FXSYS_round(rectf.top),
                   FXSYS_round(rectf.right), FXSYS_round(rectf.bottom));
      CFX_FxgeDevice device;
      device.Attach(&bmp);
      device.FillRect(&rect,
                      m_info.fillColor->m_info.pattern->m_hatchInfo.backArgb);
      for (int32_t j = rect.bottom; j < rect.top; j += mask.GetHeight()) {
        for (int32_t i = rect.left; i < rect.right; i += mask.GetWidth()) {
          device.SetBitMask(
              &mask, i, j,
              m_info.fillColor->m_info.pattern->m_hatchInfo.foreArgb);
        }
      }
      break;
    }
  }
  m_renderDevice->SaveState();
  m_renderDevice->SetClip_PathFill(path->GetPathData(), (CFX_Matrix*)matrix,
                                   fillMode);
  SetDIBitsWithMatrix(&bmp, &pattern->m_matrix);
  m_renderDevice->RestoreState();
  return FX_ERR_Succeeded;
}

FX_ERR CFX_Graphics::FillPathWithShading(CFX_Path* path,
                                         FX_FillMode fillMode,
                                         CFX_Matrix* matrix) {
  CFX_DIBitmap* bitmap = m_renderDevice->GetBitmap();
  int32_t width = bitmap->GetWidth();
  int32_t height = bitmap->GetHeight();
  FX_FLOAT start_x = m_info.fillColor->m_shading->m_beginPoint.x;
  FX_FLOAT start_y = m_info.fillColor->m_shading->m_beginPoint.y;
  FX_FLOAT end_x = m_info.fillColor->m_shading->m_endPoint.x;
  FX_FLOAT end_y = m_info.fillColor->m_shading->m_endPoint.y;
  CFX_DIBitmap bmp;
  bmp.Create(width, height, FXDIB_Argb);
  m_renderDevice->GetDIBits(&bmp, 0, 0);
  int32_t pitch = bmp.GetPitch();
  FX_BOOL result = FALSE;
  switch (m_info.fillColor->m_shading->m_type) {
    case FX_SHADING_Axial: {
      FX_FLOAT x_span = end_x - start_x;
      FX_FLOAT y_span = end_y - start_y;
      FX_FLOAT axis_len_square = (x_span * x_span) + (y_span * y_span);
      for (int32_t row = 0; row < height; row++) {
        FX_DWORD* dib_buf = (FX_DWORD*)(bmp.GetBuffer() + row * pitch);
        for (int32_t column = 0; column < width; column++) {
          FX_FLOAT x = (FX_FLOAT)(column);
          FX_FLOAT y = (FX_FLOAT)(row);
          FX_FLOAT scale =
              (((x - start_x) * x_span) + ((y - start_y) * y_span)) /
              axis_len_square;
          if (scale < 0) {
            if (!m_info.fillColor->m_shading->m_isExtendedBegin) {
              continue;
            }
            scale = 0;
          } else if (scale > 1.0f) {
            if (!m_info.fillColor->m_shading->m_isExtendedEnd) {
              continue;
            }
            scale = 1.0f;
          }
          int32_t index = (int32_t)(scale * (FX_SHADING_Steps - 1));
          dib_buf[column] = m_info.fillColor->m_shading->m_argbArray[index];
        }
      }
      result = TRUE;
      break;
    }
    case FX_SHADING_Radial: {
      FX_FLOAT start_r = m_info.fillColor->m_shading->m_beginRadius;
      FX_FLOAT end_r = m_info.fillColor->m_shading->m_endRadius;
      FX_FLOAT a = ((start_x - end_x) * (start_x - end_x)) +
                   ((start_y - end_y) * (start_y - end_y)) -
                   ((start_r - end_r) * (start_r - end_r));
      for (int32_t row = 0; row < height; row++) {
        FX_DWORD* dib_buf = (FX_DWORD*)(bmp.GetBuffer() + row * pitch);
        for (int32_t column = 0; column < width; column++) {
          FX_FLOAT x = (FX_FLOAT)(column);
          FX_FLOAT y = (FX_FLOAT)(row);
          FX_FLOAT b = -2 * (((x - start_x) * (end_x - start_x)) +
                             ((y - start_y) * (end_y - start_y)) +
                             (start_r * (end_r - start_r)));
          FX_FLOAT c = ((x - start_x) * (x - start_x)) +
                       ((y - start_y) * (y - start_y)) - (start_r * start_r);
          FX_FLOAT s;
          if (a == 0) {
            s = -c / b;
          } else {
            FX_FLOAT b2_4ac = (b * b) - 4 * (a * c);
            if (b2_4ac < 0) {
              continue;
            }
            FX_FLOAT root = (FXSYS_sqrt(b2_4ac));
            FX_FLOAT s1, s2;
            if (a > 0) {
              s1 = (-b - root) / (2 * a);
              s2 = (-b + root) / (2 * a);
            } else {
              s2 = (-b - root) / (2 * a);
              s1 = (-b + root) / (2 * a);
            }
            if (s2 <= 1.0f || m_info.fillColor->m_shading->m_isExtendedEnd) {
              s = (s2);
            } else {
              s = (s1);
            }
            if ((start_r) + s * (end_r - start_r) < 0) {
              continue;
            }
          }
          if (s < 0) {
            if (!m_info.fillColor->m_shading->m_isExtendedBegin) {
              continue;
            }
            s = 0;
          }
          if (s > 1.0f) {
            if (!m_info.fillColor->m_shading->m_isExtendedEnd) {
              continue;
            }
            s = 1.0f;
          }
          int index = (int32_t)(s * (FX_SHADING_Steps - 1));
          dib_buf[column] = m_info.fillColor->m_shading->m_argbArray[index];
        }
      }
      result = TRUE;
      break;
    }
    default: { result = FALSE; }
  }
  if (result) {
    m_renderDevice->SaveState();
    m_renderDevice->SetClip_PathFill(path->GetPathData(), (CFX_Matrix*)matrix,
                                     fillMode);
    SetDIBitsWithMatrix(&bmp, matrix);
    m_renderDevice->RestoreState();
  }
  return result;
}

FX_ERR CFX_Graphics::SetDIBitsWithMatrix(CFX_DIBSource* source,
                                         CFX_Matrix* matrix) {
  if (matrix->IsIdentity()) {
    m_renderDevice->SetDIBits(source, 0, 0);
  } else {
    CFX_Matrix m;
    m.Set((FX_FLOAT)source->GetWidth(), 0, 0, (FX_FLOAT)source->GetHeight(), 0,
          0);
    m.Concat(*matrix);
    int32_t left, top;
    std::unique_ptr<CFX_DIBitmap> bmp1(source->FlipImage(FALSE, TRUE));
    std::unique_ptr<CFX_DIBitmap> bmp2(
        bmp1->TransformTo((CFX_Matrix*)&m, left, top));
    m_renderDevice->SetDIBits(bmp2.get(), left, top);
  }
  return FX_ERR_Succeeded;
}

FX_ERR CFX_Graphics::CalcTextInfo(const CFX_WideString& text,
                                  FX_DWORD* charCodes,
                                  FXTEXT_CHARPOS* charPos,
                                  CFX_RectF& rect) {
  std::unique_ptr<CFX_UnicodeEncoding> encoding(
      new CFX_UnicodeEncoding(m_info.font));
  int32_t length = text.GetLength();
  FX_FLOAT penX = (FX_FLOAT)rect.left;
  FX_FLOAT penY = (FX_FLOAT)rect.top;
  FX_FLOAT left = (FX_FLOAT)(0);
  FX_FLOAT top = (FX_FLOAT)(0);
  charCodes[0] = text.GetAt(0);
  charPos[0].m_OriginX = penX + left;
  charPos[0].m_OriginY = penY + top;
  charPos[0].m_GlyphIndex = encoding->GlyphFromCharCode(charCodes[0]);
  charPos[0].m_FontCharWidth = FXSYS_round(
      m_info.font->GetGlyphWidth(charPos[0].m_GlyphIndex) * m_info.fontHScale);
  charPos[0].m_bGlyphAdjust = TRUE;
  charPos[0].m_AdjustMatrix[0] = -1;
  charPos[0].m_AdjustMatrix[1] = 0;
  charPos[0].m_AdjustMatrix[2] = 0;
  charPos[0].m_AdjustMatrix[3] = 1;
  penX += (FX_FLOAT)(charPos[0].m_FontCharWidth) * m_info.fontSize / 1000 +
          m_info.fontSpacing;
  for (int32_t i = 1; i < length; i++) {
    charCodes[i] = text.GetAt(i);
    charPos[i].m_OriginX = penX + left;
    charPos[i].m_OriginY = penY + top;
    charPos[i].m_GlyphIndex = encoding->GlyphFromCharCode(charCodes[i]);
    charPos[i].m_FontCharWidth =
        FXSYS_round(m_info.font->GetGlyphWidth(charPos[i].m_GlyphIndex) *
                    m_info.fontHScale);
    charPos[i].m_bGlyphAdjust = TRUE;
    charPos[i].m_AdjustMatrix[0] = -1;
    charPos[i].m_AdjustMatrix[1] = 0;
    charPos[i].m_AdjustMatrix[2] = 0;
    charPos[i].m_AdjustMatrix[3] = 1;
    penX += (FX_FLOAT)(charPos[i].m_FontCharWidth) * m_info.fontSize / 1000 +
            m_info.fontSpacing;
  }
  rect.width = (FX_FLOAT)penX - rect.left;
  rect.height = rect.top + m_info.fontSize * m_info.fontHScale - rect.top;
  return FX_ERR_Succeeded;
}

CFX_Graphics::TInfo::TInfo(const TInfo& info)
    : graphState(info.graphState),
      isAntialiasing(info.isAntialiasing),
      strokeAlignment(info.strokeAlignment),
      CTM(info.CTM),
      isActOnDash(info.isActOnDash),
      strokeColor(info.strokeColor),
      fillColor(info.fillColor),
      font(info.font),
      fontSize(info.fontSize),
      fontHScale(info.fontHScale),
      fontSpacing(info.fontSpacing) {}

CFX_Graphics::TInfo& CFX_Graphics::TInfo::operator=(const TInfo& other) {
  graphState.Copy(other.graphState);
  isAntialiasing = other.isAntialiasing;
  strokeAlignment = other.strokeAlignment;
  CTM = other.CTM;
  isActOnDash = other.isActOnDash;
  strokeColor = other.strokeColor;
  fillColor = other.fillColor;
  font = other.font;
  fontSize = other.fontSize;
  fontHScale = other.fontHScale;
  fontSpacing = other.fontSpacing;
  return *this;
}

CAGG_Graphics::CAGG_Graphics() {
  m_owner = nullptr;
}

FX_ERR CAGG_Graphics::Create(CFX_Graphics* owner,
                             int32_t width,
                             int32_t height,
                             FXDIB_Format format) {
  if (owner->m_renderDevice)
    return FX_ERR_Parameter_Invalid;
  if (m_owner)
    return FX_ERR_Property_Invalid;

  CFX_FxgeDevice* device = new CFX_FxgeDevice;
  device->Create(width, height, format);
  m_owner = owner;
  m_owner->m_renderDevice = device;
  m_owner->m_renderDevice->GetBitmap()->Clear(0xFFFFFFFF);
  return FX_ERR_Succeeded;
}

CAGG_Graphics::~CAGG_Graphics() {
  if (m_owner->m_renderDevice)
    delete (CFX_FxgeDevice*)m_owner->m_renderDevice;
  m_owner = nullptr;
}

CFX_Path::CFX_Path() {
  m_generator = nullptr;
}

FX_ERR CFX_Path::Create() {
  if (m_generator)
    return FX_ERR_Property_Invalid;

  m_generator = new CFX_PathGenerator;
  m_generator->Create();
  return FX_ERR_Succeeded;
}

CFX_Path::~CFX_Path() {
  delete m_generator;
}

FX_ERR CFX_Path::MoveTo(FX_FLOAT x, FX_FLOAT y) {
  if (!m_generator)
    return FX_ERR_Property_Invalid;
  m_generator->MoveTo(x, y);
  return FX_ERR_Succeeded;
}

FX_ERR CFX_Path::LineTo(FX_FLOAT x, FX_FLOAT y) {
  if (!m_generator)
    return FX_ERR_Property_Invalid;
  m_generator->LineTo(x, y);
  return FX_ERR_Succeeded;
}

FX_ERR CFX_Path::BezierTo(FX_FLOAT ctrlX1,
                          FX_FLOAT ctrlY1,
                          FX_FLOAT ctrlX2,
                          FX_FLOAT ctrlY2,
                          FX_FLOAT toX,
                          FX_FLOAT toY) {
  if (!m_generator)
    return FX_ERR_Property_Invalid;
  m_generator->BezierTo(ctrlX1, ctrlY1, ctrlX2, ctrlY2, toX, toY);
  return FX_ERR_Succeeded;
}

FX_ERR CFX_Path::ArcTo(FX_FLOAT left,
                       FX_FLOAT top,
                       FX_FLOAT width,
                       FX_FLOAT height,
                       FX_FLOAT startAngle,
                       FX_FLOAT sweepAngle) {
  if (!m_generator)
    return FX_ERR_Property_Invalid;
  m_generator->ArcTo(left + width / 2, top + height / 2, width / 2, height / 2,
                     startAngle, sweepAngle);
  return FX_ERR_Succeeded;
}

FX_ERR CFX_Path::Close() {
  if (!m_generator)
    return FX_ERR_Property_Invalid;
  m_generator->Close();
  return FX_ERR_Succeeded;
}

FX_ERR CFX_Path::AddLine(FX_FLOAT x1, FX_FLOAT y1, FX_FLOAT x2, FX_FLOAT y2) {
  if (!m_generator)
    return FX_ERR_Property_Invalid;
  m_generator->AddLine(x1, y1, x2, y2);
  return FX_ERR_Succeeded;
}

FX_ERR CFX_Path::AddBezier(FX_FLOAT startX,
                           FX_FLOAT startY,
                           FX_FLOAT ctrlX1,
                           FX_FLOAT ctrlY1,
                           FX_FLOAT ctrlX2,
                           FX_FLOAT ctrlY2,
                           FX_FLOAT endX,
                           FX_FLOAT endY) {
  if (!m_generator)
    return FX_ERR_Property_Invalid;
  m_generator->AddBezier(startX, startY, ctrlX1, ctrlY1, ctrlX2, ctrlY2, endX,
                         endY);
  return FX_ERR_Succeeded;
}

FX_ERR CFX_Path::AddRectangle(FX_FLOAT left,
                              FX_FLOAT top,
                              FX_FLOAT width,
                              FX_FLOAT height) {
  if (!m_generator)
    return FX_ERR_Property_Invalid;
  m_generator->AddRectangle(left, top, left + width, top + height);
  return FX_ERR_Succeeded;
}

FX_ERR CFX_Path::AddEllipse(FX_FLOAT left,
                            FX_FLOAT top,
                            FX_FLOAT width,
                            FX_FLOAT height) {
  if (!m_generator)
    return FX_ERR_Property_Invalid;
  m_generator->AddEllipse(left + width / 2, top + height / 2, width / 2,
                          height / 2);
  return FX_ERR_Succeeded;
}

FX_ERR CFX_Path::AddEllipse(const CFX_RectF& rect) {
  if (!m_generator)
    return FX_ERR_Property_Invalid;
  m_generator->AddEllipse(rect.left + rect.Width() / 2,
                          rect.top + rect.Height() / 2, rect.Width() / 2,
                          rect.Height() / 2);
  return FX_ERR_Succeeded;
}

FX_ERR CFX_Path::AddArc(FX_FLOAT left,
                        FX_FLOAT top,
                        FX_FLOAT width,
                        FX_FLOAT height,
                        FX_FLOAT startAngle,
                        FX_FLOAT sweepAngle) {
  if (!m_generator)
    return FX_ERR_Property_Invalid;
  m_generator->AddArc(left + width / 2, top + height / 2, width / 2, height / 2,
                      startAngle, sweepAngle);
  return FX_ERR_Succeeded;
}

FX_ERR CFX_Path::AddPie(FX_FLOAT left,
                        FX_FLOAT top,
                        FX_FLOAT width,
                        FX_FLOAT height,
                        FX_FLOAT startAngle,
                        FX_FLOAT sweepAngle) {
  if (!m_generator)
    return FX_ERR_Property_Invalid;
  m_generator->AddPie(left + width / 2, top + height / 2, width / 2, height / 2,
                      startAngle, sweepAngle);
  return FX_ERR_Succeeded;
}

FX_ERR CFX_Path::AddSubpath(CFX_Path* path) {
  if (!m_generator)
    return FX_ERR_Property_Invalid;
  m_generator->AddPathData(path->GetPathData());
  return FX_ERR_Succeeded;
}

FX_ERR CFX_Path::Clear() {
  if (!m_generator)
    return FX_ERR_Property_Invalid;
  m_generator->GetPathData()->SetPointCount(0);
  return FX_ERR_Succeeded;
}

FX_BOOL CFX_Path::IsEmpty() {
  if (!m_generator)
    return FX_ERR_Property_Invalid;
  if (m_generator->GetPathData()->GetPointCount() == 0) {
    return TRUE;
  }
  return FALSE;
}

CFX_PathData* CFX_Path::GetPathData() {
  if (!m_generator)
    return nullptr;
  return m_generator->GetPathData();
}

CFX_Color::CFX_Color() : m_type(FX_COLOR_None) {}

CFX_Color::CFX_Color(const FX_ARGB argb) {
  Set(argb);
}

CFX_Color::CFX_Color(CFX_Pattern* pattern, const FX_ARGB argb) {
  Set(pattern, argb);
}

CFX_Color::CFX_Color(CFX_Shading* shading) {
  Set(shading);
}

CFX_Color::~CFX_Color() {
  m_type = FX_COLOR_None;
}

FX_ERR CFX_Color::Set(const FX_ARGB argb) {
  m_type = FX_COLOR_Solid;
  m_info.argb = argb;
  m_info.pattern = nullptr;
  return FX_ERR_Succeeded;
}

FX_ERR CFX_Color::Set(CFX_Pattern* pattern, const FX_ARGB argb) {
  if (!pattern)
    return FX_ERR_Parameter_Invalid;
  m_type = FX_COLOR_Pattern;
  m_info.argb = argb;
  m_info.pattern = pattern;
  return FX_ERR_Succeeded;
}

FX_ERR CFX_Color::Set(CFX_Shading* shading) {
  if (!shading)
    return FX_ERR_Parameter_Invalid;
  m_type = FX_COLOR_Shading;
  m_shading = shading;
  return FX_ERR_Succeeded;
}

CFX_Pattern::CFX_Pattern() {
  m_type = FX_PATTERN_None;
  m_matrix.SetIdentity();
}

FX_ERR CFX_Pattern::Create(CFX_DIBitmap* bitmap,
                           const FX_FLOAT xStep,
                           const FX_FLOAT yStep,
                           CFX_Matrix* matrix) {
  if (!bitmap)
    return FX_ERR_Parameter_Invalid;
  if (m_type != FX_PATTERN_None) {
    return FX_ERR_Property_Invalid;
  }
  m_type = FX_PATTERN_Bitmap;
  m_bitmapInfo.bitmap = bitmap;
  m_bitmapInfo.x1Step = xStep;
  m_bitmapInfo.y1Step = yStep;
  if (matrix) {
    m_matrix.Set(matrix->a, matrix->b, matrix->c, matrix->d, matrix->e,
                 matrix->f);
  }
  return FX_ERR_Succeeded;
}

FX_ERR CFX_Pattern::Create(FX_HatchStyle hatchStyle,
                           const FX_ARGB foreArgb,
                           const FX_ARGB backArgb,
                           CFX_Matrix* matrix) {
  if (hatchStyle < FX_HATCHSTYLE_Horizontal ||
      hatchStyle > FX_HATCHSTYLE_SolidDiamond) {
    return FX_ERR_Parameter_Invalid;
  }
  if (m_type != FX_PATTERN_None) {
    return FX_ERR_Property_Invalid;
  }
  m_type = FX_PATTERN_Hatch;
  m_hatchInfo.hatchStyle = hatchStyle;
  m_hatchInfo.foreArgb = foreArgb;
  m_hatchInfo.backArgb = backArgb;
  if (matrix) {
    m_matrix.Set(matrix->a, matrix->b, matrix->c, matrix->d, matrix->e,
                 matrix->f);
  }
  return FX_ERR_Succeeded;
}

CFX_Pattern::~CFX_Pattern() {
  m_type = FX_PATTERN_None;
}

CFX_Shading::CFX_Shading() {
  m_type = FX_SHADING_None;
}

FX_ERR CFX_Shading::CreateAxial(const CFX_PointF& beginPoint,
                                const CFX_PointF& endPoint,
                                FX_BOOL isExtendedBegin,
                                FX_BOOL isExtendedEnd,
                                const FX_ARGB beginArgb,
                                const FX_ARGB endArgb) {
  if (m_type != FX_SHADING_None) {
    return FX_ERR_Property_Invalid;
  }
  m_type = FX_SHADING_Axial;
  m_beginPoint = beginPoint;
  m_endPoint = endPoint;
  m_isExtendedBegin = isExtendedBegin;
  m_isExtendedEnd = isExtendedEnd;
  m_beginArgb = beginArgb;
  m_endArgb = endArgb;
  return InitArgbArray();
}

FX_ERR CFX_Shading::CreateRadial(const CFX_PointF& beginPoint,
                                 const CFX_PointF& endPoint,
                                 const FX_FLOAT beginRadius,
                                 const FX_FLOAT endRadius,
                                 FX_BOOL isExtendedBegin,
                                 FX_BOOL isExtendedEnd,
                                 const FX_ARGB beginArgb,
                                 const FX_ARGB endArgb) {
  if (m_type != FX_SHADING_None) {
    return FX_ERR_Property_Invalid;
  }
  m_type = FX_SHADING_Radial;
  m_beginPoint = beginPoint;
  m_endPoint = endPoint;
  m_beginRadius = beginRadius;
  m_endRadius = endRadius;
  m_isExtendedBegin = isExtendedBegin;
  m_isExtendedEnd = isExtendedEnd;
  m_beginArgb = beginArgb;
  m_endArgb = endArgb;
  return InitArgbArray();
}

CFX_Shading::~CFX_Shading() {
  m_type = FX_SHADING_None;
}

FX_ERR CFX_Shading::InitArgbArray() {
  int32_t a1, r1, g1, b1;
  ArgbDecode(m_beginArgb, a1, r1, g1, b1);
  int32_t a2, r2, g2, b2;
  ArgbDecode(m_endArgb, a2, r2, g2, b2);
  FX_FLOAT f = (FX_FLOAT)(FX_SHADING_Steps - 1);
  FX_FLOAT aScale = (FX_FLOAT)(1.0 * (a2 - a1) / f);
  FX_FLOAT rScale = (FX_FLOAT)(1.0 * (r2 - r1) / f);
  FX_FLOAT gScale = (FX_FLOAT)(1.0 * (g2 - g1) / f);
  FX_FLOAT bScale = (FX_FLOAT)(1.0 * (b2 - b1) / f);
  int32_t a3, r3, g3, b3;
  for (int32_t i = 0; i < FX_SHADING_Steps; i++) {
    a3 = (int32_t)(i * aScale);
    r3 = (int32_t)(i * rScale);
    g3 = (int32_t)(i * gScale);
    b3 = (int32_t)(i * bScale);
    m_argbArray[i] =
        FXARGB_TODIB(FXARGB_MAKE((a1 + a3), (r1 + r3), (g1 + g3), (b1 + b3)));
  }
  return FX_ERR_Succeeded;
}
