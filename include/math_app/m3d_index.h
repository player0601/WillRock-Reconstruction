// m3d_index.h -- generated index of every m3d library function (feb map) with its v12 twin.
// Columns: pretty name | v12 | feb | match kind (exact = byte-identical modulo relocs, i.e. verified;
// callgraph/vtable = automatic, spot-checked only; '-' = no automatic twin, search by prologue).
// Not compilable: a lookup table in C-comment form. See m3d.h for the typed declarations.
// "bytes" = located by prologue byte similarity next to exact neighbours (>=0.75 of the first 24 bytes equal).


// ---- m3d_math.obj ----
//   m3dSolveQuadraticEq                          v12 0x00565B30 (feb 0x00549350)  callgraph ?m3dSolveQuadraticEq@@YIHMMMPAM0@Z
//   m3dUpdateTimeField                           v12 0x00565D20 (feb 0x00549550)  exact     ?m3dUpdateTimeField@@YIHPAMM@Z
//   m3dSolveLinSys2D                             v12 0x00565D70 (feb 0x005495A0)  exact     ?m3dSolveLinSys2D@@YIHMMMMMMPAM0@Z
//   m3dLimitChange_Float                         v12 0x00565E10 (feb 0x00549640)  exact     ?m3dLimitChange_Float@@YIHMMMPAM@Z

// ---- m3d_matr.obj ----
//   m3dInit3DMatr                                v12 0x005609A0 (feb 0x005496C0)  exact     ?m3dInit3DMatr@@YIHXZ
//   m3dAddMatr                                   v12 0x00560A20 (feb 0x00549740)  exact     ?m3dAddMatr@@YIXPAVm3dMATR@@00@Z
//   m3dMultiplyMatr                              v12 0x00560A50 (feb 0x00549770)  callgraph ?m3dMultiplyMatr@@YAXPAVm3dMATR@@00@Z
//   m3dIsEqualMatr_A                             v12 0x00560DF0 (feb 0x00549B10)  exact     ?m3dIsEqualMatr_A@@YIHPAVm3dMATR@@0MM@Z
//   m3dMATR::Identity                            v12 0x00561050 (feb 0x00549BA0)  exact     ?Identity@m3dMATR@@QAEXXZ
//   m3dMATR::Translate                           v12 0x00561080 (feb 0x00549BD0)  exact     ?Translate@m3dMATR@@QAEXPAUm3dV@@H@Z
//   m3dMATR::Translate                           v12 0x005610A0 (feb 0x00549BF0)  exact     ?Translate@m3dMATR@@QAEXMMMH@Z
//   m3dMATR::RotateX                             v12 0x00561170 (feb 0x00549CC0)  callgraph ?RotateX@m3dMATR@@QAEXMH@Z
//   m3dMATR::RotateY                             v12 0x00561210 (feb 0x00549D60)  exact     ?RotateY@m3dMATR@@QAEXMH@Z
//   m3dMATR::RotateZ                             v12 0x005612B0 (feb 0x00549E00)  bytes     ?RotateZ@m3dMATR@@QAEXMH@Z
//   m3dMATR::Scale                               v12 0x00561350 (feb 0x00549EA0)  exact     ?Scale@m3dMATR@@QAEXMMMH@Z
//   m3dMATR::Transform                           v12 0x00561420 (feb 0x00549F70)  exact     ?Transform@m3dMATR@@QAEXPAV1@H@Z
//   m3dMATR::GetOrigin                           v12 0x00561480 (feb 0x00549FD0)  callgraph ?GetOrigin@m3dMATR@@QAEXPAUm3dV@@@Z
//   m3dMATR::GetAxisX                            v12 0x005614A0 (feb 0x00549FF0)  callgraph ?GetAxisX@m3dMATR@@QAEXPAUm3dV@@@Z
//   m3dMATR::GetAxisY                            v12 0x005614C0 (feb 0x0054A010)  callgraph ?GetAxisY@m3dMATR@@QAEXPAUm3dV@@@Z
//   m3dMATR::GetAxisZ                            v12 0x005614E0 (feb 0x0054A030)  callgraph ?GetAxisZ@m3dMATR@@QAEXPAUm3dV@@@Z
//   m3dMATR::GetTranslate                        v12 0x00561500 (feb 0x0054A050)  exact     ?GetTranslate@m3dMATR@@QAEXPAM00@Z
//   m3dMATR::GetScale                            v12 0x00561520 (feb 0x0054A070)  callgraph ?GetScale@m3dMATR@@QAEXPAM00@Z
//   m3dMATR::MakeRotY                            v12 0x00562000 (feb 0x0054AAE0)  exact     ?MakeRotY@m3dMATR@@QAEXM@Z
//   m3dMATR::MakeRot                             v12 0x00562050 (feb 0x0054AB30)  callgraph ?MakeRot@m3dMATR@@QAEXPAUm3dV@@0@Z
//   m3dMATR::MakeRot                             v12 0x00562230 (feb 0x0054AD10)  callgraph ?MakeRot@m3dMATR@@QAEXPAUm3dV@@00@Z
//   m3dMATR::MakeRotRay                          v12 0x00562450 (feb 0x0054AF30)  exact     ?MakeRotRay@m3dMATR@@QAEXPAUm3dV@@0M@Z
//   m3dMATR::MakeLCS2WCS                         v12 0x005624C0 (feb 0x0054AFA0)  exact     ?MakeLCS2WCS@m3dMATR@@QAEXPAUm3dV@@000@Z
//   m3dMATR::_MakeWCS2LCS_VZ                     v12 0x00562600 (feb 0x0054B0E0)  callgraph ?_MakeWCS2LCS_VZ@m3dMATR@@QAEXPAUm3dV@@0@Z
//   m3dMATR::MakeWCS2LCS                         v12 0x00562730 (feb 0x0054B210)  exact     ?MakeWCS2LCS@m3dMATR@@QAEXPAUm3dV@@000@Z
//   m3dMATR::MakeLCS2WCS_VZ                      v12 0x00562870 (feb 0x0054B350)  exact     ?MakeLCS2WCS_VZ@m3dMATR@@QAEXPAUm3dV@@0@Z
//   m3dMATR::_MakeLCS2WCS_VX                     v12 0x005628F0 (feb 0x0054B3D0)  exact     ?_MakeLCS2WCS_VX@m3dMATR@@QAEXPAUm3dV@@0@Z
//   m3dMATR::_MakeLCS2WCS_VZ                     v12 0x00562A20 (feb 0x0054B500)  callgraph ?_MakeLCS2WCS_VZ@m3dMATR@@QAEXPAUm3dV@@0@Z
//   m3dMATR::_MakeLCS2WCS_VY                     v12 0x00562B50 (feb 0x0054B630)  exact     ?_MakeLCS2WCS_VY@m3dMATR@@QAEXPAUm3dV@@0@Z
//   m3dMATR::Orthonormalize                      v12 0x00562C60 (feb 0x0054B740)  exact     ?Orthonormalize@m3dMATR@@QAEXXZ
//   m3dMATR::MakeProjTri                         v12 0x00562D80 (feb 0x0054B860)  exact     ?MakeProjTri@m3dMATR@@QAEXPAUm3dV@@00@Z
//   m3dMATR::MakePolyPoly                        v12 0x00562F00 (feb 0x0054B9E0)  callgraph ?MakePolyPoly@m3dMATR@@QAEHPAVm3dPOLY@@0@Z
//   m3dMATR::Transpose                           v12 0x005634B0 (feb 0x0054BF90)  exact     ?Transpose@m3dMATR@@QAEXXZ
//   m3dMATR::Invert                              v12 0x00563500 (feb 0x0054BFE0)  callgraph ?Invert@m3dMATR@@QAEHPAV1@@Z
//   m3dMATR::RemoveScale                         v12 0x00563700 (feb 0x0054C1E0)  bytes     ?RemoveScale@m3dMATR@@QAEXXZ
//   m3dMATR::TransformPointList                  v12 0x00564190 (feb 0x0054CC70)  callgraph ?TransformPointList@m3dMATR@@QAAXHPAXH0H@Z
//   m3dMATR::TransformVectorList                 v12 0x00564600 (feb 0x0054D0E0)  callgraph ?TransformVectorList@m3dMATR@@QAAXHPAXH0H@Z

// ---- m3d_vect.obj ----
//   _m3dAngleVector                              v12 0x00564DA0 (feb 0x0054D600)  exact     ?_m3dAngleVector@@YIMPAUm3dV@@0@Z
//   m3dAngleVector_COS                           v12 0x00564E10 (feb 0x0054D670)  callgraph ?m3dAngleVector_COS@@YIMPAUm3dV@@0@Z
//   m3dAngleVector                               v12 0x00565010 (feb 0x0054D870)  callgraph ?m3dAngleVector@@YIMPAUm3dV@@0@Z
//   m3xzAngleVector                              v12 0x00565220 (feb 0x0054DA90)  callgraph ?m3xzAngleVector@@YIMPAUm3dV@@0@Z
//   m3dBasisCoord                                v12 0x005654C0 (feb 0x0054DD20)  callgraph ?m3dBasisCoord@@YIXPAUm3dV@@00PAM1@Z
//   m2dAngleVector                               v12 0x005658C0 (feb 0x0054E140)  exact     ?m2dAngleVector@@YIMPAUm2dV@@0@Z
//   m3dMirrorPointPlane                          v12 0x005659B0 (feb 0x0054E180)  exact     ?m3dMirrorPointPlane@@YIXPAUm3dV@@000@Z
//   m3dReflectVector                             v12 0x00565A40 (feb 0x0054E210)  exact     ?m3dReflectVector@@YIXPAUm3dV@@00@Z
//   m3dWCS2SphereCoord                           v12 0x00565A80 (feb 0x0054E250)  exact     ?m3dWCS2SphereCoord@@YIXPAUm3dV@@PAM1@Z

// ---- m3d_spl.obj ----
//   m3dSPL::Make                                 v12 0x00565E90 (feb 0x0054E300)  callgraph ?Make@m3dSPL@@SIPAV1@H@Z
//   m3dSPL::Make                                 v12 0x00566090 (feb 0x0054E500)  exact     ?Make@m3dSPL@@SIPAV1@HH@Z
//   m3dSPL::AllocKpList                          v12 0x005660F0 (feb 0x0054E560)  exact     ?AllocKpList@m3dSPL@@QAEHH@Z
//   m3dSPL::Duplicate                            v12 0x00566140 (feb 0x0054E5B0)  exact     ?Duplicate@m3dSPL@@UAEPAV1@XZ
//   m3dSPL::RemoveKp                             v12 0x00566230 (feb 0x0054E6A0)  exact     ?RemoveKp@m3dSPL@@QAEHHH@Z
//   m3dSPL::GetKp                                v12 0x005662E0 (feb 0x0054E750)  exact     ?GetKp@m3dSPL@@QAEXHPAX@Z
//   m3dSPL::FindSeg                              v12 0x005664E0 (feb 0x0054E950)  exact     ?FindSeg@m3dSPL@@QAEHM@Z
//   m3dSPL::Optimize                             v12 0x00566600 (feb 0x0054EA70)  exact     ?Optimize@m3dSPL@@QAEHH@Z
//   m3dSPL::IsIntegerArg                         v12 0x00566B70 (feb 0x0054EFE0)  exact     ?IsIntegerArg@m3dSPL@@QAEHXZ
//   m3dSPL::IsConst                              v12 0x00566C10 (feb 0x0054F080)  exact     ?IsConst@m3dSPL@@QAEHXZ
//   m3dSPL::CompareKP                            v12 0x00566C80 (feb 0x0054F0F0)  exact     ?CompareKP@m3dSPL@@MAEHPAVm3dSPL_KP@@0@Z
//   m3dSPL::Sample                               v12 0x00566CB0 (feb 0x0054F120)  exact     ?Sample@m3dSPL@@UAEPAV1@MMMH@Z
//   m3dSPL::GetMinMaxValue                       v12 0x00566DB0 (feb 0x0054F220)  exact     ?GetMinMaxValue@m3dSPL@@UAEXMMMPAX0@Z
//   m3dSPL_LINEAR1D::GetValue                    v12 0x00566ED0 (feb 0x0054F340)  vtable    ?GetValue@m3dSPL_LINEAR1D@@UAEXMPAX0@Z
//   m3dSPL_LINEAR1D::SetKp                       v12 0x00567470 (feb 0x0054F4B0)  exact     ?SetKp@m3dSPL_LINEAR1D@@QAEXHMM@Z
//   m3dSPL_LINEAR1D::CompareKP                   v12 0x005674B0 (feb 0x0054F4F0)  exact     ?CompareKP@m3dSPL_LINEAR1D@@MAEHPAVm3dSPL_KP@@0@Z
//   m3dSPL_LINEAR2D::GetValue                    v12 0x00567510 (feb 0x0054F550)  exact     ?GetValue@m3dSPL_LINEAR2D@@UAEXMPAX0@Z
//   m3dSPL_LINEAR2D::CompareKP                   v12 0x00567710 (feb 0x0054F750)  exact     ?CompareKP@m3dSPL_LINEAR2D@@MAEHPAVm3dSPL_KP@@0@Z
//   m3dSPL_LINEAR3D::GetValue                    v12 0x005677B0 (feb 0x0054F7F0)  exact     ?GetValue@m3dSPL_LINEAR3D@@UAEXMPAX0@Z
//   m3dSPL_LINEAR3D::GetValueClosest             v12 0x00567A60 (feb 0x0054FAA0)  exact     ?GetValueClosest@m3dSPL_LINEAR3D@@UAEXMPAX0@Z
//   m3dSPL_LINEAR3D::CompareKP                   v12 0x00567B50 (feb 0x0054FB90)  exact     ?CompareKP@m3dSPL_LINEAR3D@@MAEHPAVm3dSPL_KP@@0@Z
//   m3dSPL_LINEAR3D::DistPoint                   v12 0x00567C40 (feb 0x0054FC80)  exact     ?DistPoint@m3dSPL_LINEAR3D@@UAEMPAXPAHPAM0@Z
//   m3dSPL_HERMIT::GetValue                      v12 0x00567D60 (feb 0x0054FDA0)  exact     ?GetValue@m3dSPL_HERMIT@@UAEXMPAX0@Z
//   m3dSPL_LAGRANGE::GetValue                    v12 0x00567ED0 (feb 0x0054FF10)  exact     ?GetValue@m3dSPL_LAGRANGE@@UAEXMPAX0@Z
//   m3dSPL_BEZIER::GetValueDim                   v12 0x005680A0 (feb 0x005500E0)  exact     ?GetValueDim@m3dSPL_BEZIER@@IAEXMHPAX00000@Z
//   m3dSPL_BEZIER2D::GetValue                    v12 0x00568250 (feb 0x00550290)  exact     ?GetValue@m3dSPL_BEZIER2D@@UAEXMPAX0@Z
//   m3dSPL_BEZIER2D::SetKp                       v12 0x005682E0 (feb 0x00550320)  exact     ?SetKp@m3dSPL_BEZIER2D@@QAEXHMPAUm2dV@@00@Z
//   m3dSPL_BEZIER3D::GetValue                    v12 0x00568370 (feb 0x005503B0)  exact     ?GetValue@m3dSPL_BEZIER3D@@UAEXMPAX0@Z
//   m3dSPL_BEZIER3D::Convert                     v12 0x00568400 (feb 0x00550440)  vtable    ?Convert@m3dSPL_BEZIER3D@@UAEPAVm3dSPL@@HH@Z
//   m3dSPL_QUAT::GetValue                        v12 0x005687E0 (feb 0x00550830)  exact     ?GetValue@m3dSPL_QUAT@@UAEXMPAX0@Z
//   m3dSPL_QUAT::GetValueClosest                 v12 0x005688B0 (feb 0x00550900)  exact     ?GetValueClosest@m3dSPL_QUAT@@UAEXMPAX0@Z
//   m3dSPL_QUAT::CompareKP                       v12 0x00568990 (feb 0x005509E0)  exact     ?CompareKP@m3dSPL_QUAT@@MAEHPAVm3dSPL_KP@@0@Z
//   m3dSPL_COLOR::GetValue                       v12 0x005689C0 (feb 0x00550A10)  exact     ?GetValue@m3dSPL_COLOR@@UAEXMPAX0@Z
//   m3dSPL_COLOR::SetKp                          v12 0x00568D50 (feb 0x00550DA0)  exact     ?SetKp@m3dSPL_COLOR@@QAEXHMMMMM@Z
//   m3dSPL_COLOR::CompareKP                      v12 0x00568DE0 (feb 0x00550E30)  exact     ?CompareKP@m3dSPL_COLOR@@MAEHPAVm3dSPL_KP@@0@Z

// ---- m3d_ctrl.obj ----
//   m3dCTRL_TIME::Update                         v12 0x005691C0 (feb 0x00551130)  exact     ?Update@m3dCTRL_TIME@@UAEXM@Z
//   m3dCTRL_1D_SPL::GetValue                     v12 0x00569220 (feb 0x00551190)  exact     ?GetValue@m3dCTRL_1D_SPL@@UAEXPAM@Z
//   m3dCTRL_1D_SPL::GetMaxValue                  v12 0x00569330 (feb 0x005512A0)  exact     ?GetMaxValue@m3dCTRL_1D_SPL@@UAEMXZ
//   m3dCTRL_COLOR_SPL::GetValue                  v12 0x005693A0 (feb 0x00551310)  exact     ?GetValue@m3dCTRL_COLOR_SPL@@UAEXPAVm3dCOLOR@@@Z
//   m3dCTRL_1D_NOISE::GetValue                   v12 0x005694B0 (feb 0x00551420)  exact     ?GetValue@m3dCTRL_1D_NOISE@@UAEXPAM@Z
//   m3dCTRL_1D_NOISE::GetMaxValue                v12 0x00569510 (feb 0x00551480)  vtable    ?GetMaxValue@m3dCTRL_1D_NOISE@@UAEMXZ
//   m3dCTRL_COLOR_NOISE::GetValue                v12 0x00569520 (feb 0x00551490)  exact     ?GetValue@m3dCTRL_COLOR_NOISE@@UAEXPAVm3dCOLOR@@@Z

// ---- m3d.obj ----
//   m3dInit                                      v12 0x005695C0 (feb 0x00551530)  exact     ?m3dInit@@YIHXZ
//   m3dTerm                                      v12 ?????????? (feb 0x00551560)  -         ?m3dTerm@@YIXXZ
//   m3dInitHW                                    v12 0x00569600 (feb 0x00551570)  callgraph ?m3dInitHW@@YIHXZ
//   _m3dCheckValid                               v12 0x00569640 (feb 0x00551580)  exact     ?_m3dCheckValid@@YIHM@Z
//   _m3dCheckValid                               v12 0x005696A0 (feb 0x005515E0)  exact     ?_m3dCheckValid@@YIHPBUm3dVTX@@@Z
//   m3dGetSimdType                               v12 0x00569880 (feb 0x005517C0)  callgraph ?m3dGetSimdType@@YIHXZ

// ---- cam.obj ----
//   ?0camCAMERA                                  v12 0x00569B00 (feb 0x005517F0)  callgraph ??0camCAMERA@@QAE@XZ
//   ?1camCAMERA                                  v12 0x00569DC0 (feb 0x00551AB0)  exact     ??1camCAMERA@@QAE@XZ
//   camCAMERA::SetViewportDx                     v12 0x00569E10 (feb 0x00551B00)  exact     ?SetViewportDx@camCAMERA@@QAEXM@Z
//   camCAMERA::SetViewportDy                     v12 0x00569E70 (feb 0x00551B60)  exact     ?SetViewportDy@camCAMERA@@QAEXM@Z
//   camCAMERA::SetAspectRatioHW                  v12 0x00569EF0 (feb 0x00551BE0)  exact     ?SetAspectRatioHW@camCAMERA@@QAEXM@Z
//   camCAMERA::SetAngleHor                       v12 0x00569FF0 (feb 0x00551CE0)  exact     ?SetAngleHor@camCAMERA@@QAEXM@Z
//   camCAMERA::SetAngleVert                      v12 0x0056A120 (feb 0x00551E10)  exact     ?SetAngleVert@camCAMERA@@QAEXM@Z
//   camCAMERA::SetNearPlane                      v12 0x0056A250 (feb 0x00551F40)  exact     ?SetNearPlane@camCAMERA@@QAEXM@Z
//   camCAMERA::SetViewWindow                     v12 0x0056A350 (feb 0x00552040)  exact     ?SetViewWindow@camCAMERA@@QAEXMMMM@Z
//   camCAMERA::SetViewPort                       v12 0x0056A3A0 (feb 0x00552090)  exact     ?SetViewPort@camCAMERA@@QAEXMMMM@Z
//   camCAMERA::MoveTo                            v12 0x0056A440 (feb 0x00552130)  callgraph ?MoveTo@camCAMERA@@QAEXPAUm3dV@@@Z
//   camCAMERA::MoveRelForw                       v12 ?????????? (feb 0x005522F0)  -         ?MoveRelForw@camCAMERA@@QAEXM@Z
//   camCAMERA::MoveRelSide                       v12 ?????????? (feb 0x005524B0)  -         ?MoveRelSide@camCAMERA@@QAEXM@Z
//   camCAMERA::MoveRelUp                         v12 ?????????? (feb 0x00552660)  -         ?MoveRelUp@camCAMERA@@QAEXM@Z
//   camCAMERA::MoveRel                           v12 ?????????? (feb 0x00552810)  -         ?MoveRel@camCAMERA@@QAEXPAUm3dV@@@Z
//   camCAMERA::TurnRelAroundX                    v12 ?????????? (feb 0x005529D0)  -         ?TurnRelAroundX@camCAMERA@@QAEXM@Z
//   camCAMERA::TurnAroundVert                    v12 ?????????? (feb 0x00552B80)  -         ?TurnAroundVert@camCAMERA@@QAEXM@Z
//   camCAMERA::ResetVertRotation                 v12 ?????????? (feb 0x00552D60)  -         ?ResetVertRotation@camCAMERA@@QAEXXZ
//   camCAMERA::SetInterest                       v12 0x0056B2C0 (feb 0x00552FD0)  exact     ?SetInterest@camCAMERA@@QAEXPAUm3dV@@@Z
//   camCAMERA::SetLookAt                         v12 0x0056B460 (feb 0x00553170)  exact     ?SetLookAt@camCAMERA@@QAEXPAUm3dV@@@Z
//   camCAMERA::GetVUp                            v12 0x0056B5B0 (feb 0x005532C0)  callgraph ?GetVUp@camCAMERA@@QAEXPAUm3dV@@@Z
//   camCAMERA::GetVRight                         v12 0x0056B5D0 (feb 0x005532E0)  callgraph ?GetVRight@camCAMERA@@QAEXPAUm3dV@@@Z
//   camCAMERA::GetLookAt                         v12 0x0056B5F0 (feb 0x00553300)  callgraph ?GetLookAt@camCAMERA@@QAEXPAUm3dV@@@Z
//   camCAMERA::GetVForw                          v12 0x0056B5F0 (feb 0x00553300)  callgraph ?GetVForw@camCAMERA@@QAEXPAUm3dV@@@Z
//   camCAMERA::TransformPoint                    v12 0x0056B610 (feb 0x00553320)  exact     ?TransformPoint@camCAMERA@@QAEXPAUm3dV@@0@Z
//   camCAMERA::TransformPointC2W                 v12 0x0056B790 (feb 0x005534A0)  exact     ?TransformPointC2W@camCAMERA@@QAEXPAUm3dV@@0@Z
//   camCAMERA::TransformVector                   v12 0x0056B910 (feb 0x00553620)  exact     ?TransformVector@camCAMERA@@QAEXPAUm3dV@@0@Z
//   camCAMERA::GetOrigin                         v12 0x0056BA60 (feb 0x00553770)  callgraph ?GetOrigin@camCAMERA@@QAEXPAUm3dV@@@Z
//   camCAMERA::TransformMatrC2W                  v12 0x0056BA80 (feb 0x00553790)  callgraph ?TransformMatrC2W@camCAMERA@@QAEXPAVm3dMATR@@H@Z
//   camCAMERA::GetParameters                     v12 0x0056BC90 (feb 0x005539B0)  exact     ?GetParameters@camCAMERA@@QAEXPAUm3dV@@000@Z
//   camCAMERA::SetParameters                     v12 0x0056BD00 (feb 0x00553A20)  callgraph ?SetParameters@camCAMERA@@QAEXPAUm3dV@@000@Z
//   camCAMERA::GetViewWindowRayDir               v12 0x0056BF00 (feb 0x00553C20)  exact     ?GetViewWindowRayDir@camCAMERA@@QAEXMMPAUm3dV@@@Z
//   camCAMERA::Animate                           v12 ?????????? (feb 0x00553DB0)  -         ?Animate@camCAMERA@@QAEXMPAV1@@Z

// ---- m3d_vol.obj ----
//   m3dVOL::Make                                 v12 0x005698B0 (feb 0x00554260)  callgraph ?Make@m3dVOL@@SIPAV1@H@Z
//   m3dVOL::QualPlane                            v12 0x00569950 (feb 0x00554300)  exact     ?QualPlane@m3dVOL@@UAEHPAUm3dV@@0@Z
//   m3dIsBelongPointCone                         v12 ?????????? (feb 0x00554350)  -         ?m3dIsBelongPointCone@@YIHPAUm3dV@@00MMM@Z

// ---- m3d_box.obj ----
//   m3dBOX::IsBelongPoint                        v12 0x0056C530 (feb 0x005544B0)  exact     ?IsBelongPoint@m3dBOX@@UAEHPAUm3dV@@@Z
//   m3dBOX::IsIsectCyl                           v12 0x0056C5C0 (feb 0x00554540)  exact     ?IsIsectCyl@m3dBOX@@UAEHPAUm3dV@@0MM@Z
//   m3dBOX::IsectRay                             v12 0x0056C660 (feb 0x005545E0)  callgraph ?IsectRay@m3dBOX@@UAEHPAUm3dV@@0M0PAMPAH@Z
//   m3dBOX::DistPoint                            v12 0x0056C8B0 (feb 0x00554840)  callgraph ?DistPoint@m3dBOX@@UAEHPAUm3dV@@M0PAMPAH@Z
//   m3dBOX::CalcStart                            v12 0x0056CF60 (feb 0x00554F00)  exact     ?CalcStart@m3dBOX@@QAEXXZ
//   m3dBOX::CalcUpdate                           v12 0x0056CFA0 (feb 0x00554F40)  exact     ?CalcUpdate@m3dBOX@@QAEXPAUm3dV@@@Z
//   m3dBOX::CalcUpdate                           v12 0x0056D040 (feb 0x00554FE0)  exact     ?CalcUpdate@m3dBOX@@QAEXPAUm3dV@@H@Z
//   m3dBOX::CalcUpdate                           v12 0x0056D060 (feb 0x00555000)  exact     ?CalcUpdate@m3dBOX@@QAEXPAV1@@Z
//   m3dBOX::Calc                                 v12 0x0056D080 (feb 0x00555020)  exact     ?Calc@m3dBOX@@QAEXPAUm3dV@@HM@Z
//   m3dBOX::CalcXZ                               v12 0x0056D1C0 (feb 0x00555160)  exact     ?CalcXZ@m3dBOX@@QAEXPAUm3dV@@HM@Z
//   m3dBOX::MakePointPoint                       v12 0x0056D290 (feb 0x00555230)  exact     ?MakePointPoint@m3dBOX@@QAEXPAUm3dV@@0@Z
//   m3dBOX::GetVertices                          v12 0x0056D350 (feb 0x005552F0)  exact     ?GetVertices@m3dBOX@@QAEXQAUm3dV@@@Z
//   m3dBOX::GetEdgePoints                        v12 ?????????? (feb 0x00555390)  -         ?GetEdgePoints@m3dBOX@@QAEXHPAUm3dV@@0@Z
//   m3dBOX::GetFace                              v12 0x0056D620 (feb 0x005555C0)  callgraph ?GetFace@m3dBOX@@UAEXHPAVm3dPOLY@@PAUm3dV@@@Z
//   m3dBOX::Check                                v12 0x0056D870 (feb 0x00555810)  exact     ?Check@m3dBOX@@UAEHXZ
//   m3dBOX::CalcEnclSph                          v12 0x0056D8C0 (feb 0x00555860)  vtable    ?CalcEnclSph@m3dBOX@@UAEXPAVm3dSPHERE@@@Z
//   m3dBOX::CheckEnclSph                         v12 0x0056D9B0 (feb 0x00555950)  vtable    ?CheckEnclSph@m3dBOX@@UAEHPAVm3dSPHERE@@@Z
//   m3dBOX::IsBelongPointFace                    v12 0x0056DA40 (feb 0x005559E0)  callgraph ?IsBelongPointFace@m3dBOX@@AAEHHPAUm3dV@@@Z
//   m3dBOX::GetCenter                            v12 0x0056DB60 (feb 0x00555B00)  exact     ?GetCenter@m3dBOX@@UAEXPAUm3dV@@@Z
//   m3dBOX::GetArea                              v12 0x0056DBA0 (feb 0x00555B40)  exact     ?GetArea@m3dBOX@@QAEMXZ
//   m3dBOX::IsIsectBox                           v12 0x0056DBD0 (feb 0x00555B70)  exact     ?IsIsectBox@m3dBOX@@QAEHPAV1@@Z
//   m3dBOX::IsIsectPoly                          v12 0x0056DC20 (feb 0x00555BC0)  exact     ?IsIsectPoly@m3dBOX@@QAEHPAVm3dPOLY@@PAUm3dV@@@Z
//   m3dBOX::Expand                               v12 0x0056E070 (feb 0x00556010)  exact     ?Expand@m3dBOX@@UAEXM@Z
//   m3dBOX::ClipLSeg                             v12 0x0056E0B0 (feb 0x00556050)  exact     ?ClipLSeg@m3dBOX@@QAEHPAUm3dV@@000@Z

// ---- m3d_rand.obj ----
//   m3dRandom                                    v12 0x0056E720 (feb 0x00556600)  callgraph ?m3dRandom@@YIMXZ
//   m3dRandMax                                   v12 0x0056E740 (feb 0x00556620)  callgraph ?m3dRandMax@@YIMM@Z
//   m3dRandRange                                 v12 0x0056E760 (feb 0x00556640)  exact     ?m3dRandRange@@YIMMM@Z
//   m3dRandIntMax                                v12 0x0056E790 (feb 0x00556670)  exact     ?m3dRandIntMax@@YIHH@Z
//   m3dRandIntRange                              v12 0x0056E7C0 (feb 0x005566A0)  exact     ?m3dRandIntRange@@YIHHH@Z
//   m3dRND_GEN::RndIntMax                        v12 0x0056E800 (feb 0x005566E0)  exact     ?RndIntMax@m3dRND_GEN@@QAEHH@Z
//   m3dRND_GEN::RndFloat                         v12 0x0056E840 (feb 0x00556720)  exact     ?RndFloat@m3dRND_GEN@@QAEMXZ
//   m3dRND_GEN::RndFloatMax                      v12 0x0056E880 (feb 0x00556760)  exact     ?RndFloatMax@m3dRND_GEN@@QAEMM@Z
//   m3dRND_GEN::RndFloatRange                    v12 0x0056E8C0 (feb 0x005567A0)  exact     ?RndFloatRange@m3dRND_GEN@@QAEMMM@Z
//   m3dRandBasicCoord                            v12 0x0056E910 (feb 0x005567F0)  exact     ?m3dRandBasicCoord@@YIXPAM0@Z
//   m3dMakeVUnitRandom                           v12 0x0056E990 (feb 0x00556870)  exact     ?m3dMakeVUnitRandom@@YIXPAUm3dV@@@Z
//   m3dMakeVUnitConusRandom                      v12 0x0056EA00 (feb 0x005568E0)  callgraph ?m3dMakeVUnitConusRandom@@YIXPAUm3dV@@M0@Z
//   m3dMakeVTangRandom                           v12 0x0056ED00 (feb 0x00556BE0)  exact     ?m3dMakeVTangRandom@@YIXPAUm3dV@@0@Z
//   m3dMakeVCylinderRandom                       v12 0x0056EE00 (feb 0x00556CE0)  callgraph ?m3dMakeVCylinderRandom@@YIXPAUm3dV@@MM0@Z
//   m3dRandSelect                                v12 0x0056F020 (feb 0x00556F00)  exact     ?m3dRandSelect@@YIHHH@Z
//   m3dRandSelect                                v12 0x0056F040 (feb 0x00556F20)  exact     ?m3dRandSelect@@YIHHHH@Z
//   m3dRandSelect                                v12 0x0056F070 (feb 0x00556F50)  exact     ?m3dRandSelect@@YIHHHHH@Z

// ---- m3d_misc.obj ----
//   m3dGetClosestVert                            v12 ?????????? (feb 0x00556FB0)  -         ?m3dGetClosestVert@@YIHHPAUm3dV@@0@Z
//   _m3xzIsIsectPolyCircle                       v12 0x0056F1D0 (feb 0x005570C0)  exact     ?_m3xzIsIsectPolyCircle@@YIHPAVm3dPOLY@@PAUm3dV@@M@Z
//   m3xzIsIsectPolyCircle                        v12 0x0056F3A0 (feb 0x00557290)  callgraph ?m3xzIsIsectPolyCircle@@YIHPAVm3dPOLY@@PAUm3dV@@M@Z
//   _m3xzDist2PointPoly                          v12 0x0056F5A0 (feb 0x00557490)  exact     ?_m3xzDist2PointPoly@@YIXPAUm3dV@@PAVm3dPOLY@@0PAM@Z

// ---- m3d_sph.obj ----
//   m3dSPHERE::IsBelongPoint                     v12 0x0056F670 (feb 0x00557560)  callgraph ?IsBelongPoint@m3dSPHERE@@UAEHPAUm3dV@@@Z
//   m3dSPHERE::IsIsectCyl                        v12 0x0056F750 (feb 0x00557640)  exact     ?IsIsectCyl@m3dSPHERE@@UAEHPAUm3dV@@0MM@Z
//   m3dSPHERE::IsectRay                          v12 0x0056F790 (feb 0x00557680)  callgraph ?IsectRay@m3dSPHERE@@UAEHPAUm3dV@@0M0PAMPAH@Z
//   m3dSPHERE::DistPoint                         v12 0x0056FAA0 (feb 0x00557990)  vtable    ?DistPoint@m3dSPHERE@@UAEHPAUm3dV@@M0PAMPAH@Z
//   m3dSPHERE::Check                             v12 0x0056FC00 (feb 0x00557AF0)  vtable    ?Check@m3dSPHERE@@UAEHXZ
//   m3dSPHERE::CalcEnclSph                       v12 0x0056FC20 (feb 0x00557B10)  exact     ?CalcEnclSph@m3dSPHERE@@UAEXPAV1@@Z
//   m3dSPHERE::CheckEnclSph                      v12 0x0056FC50 (feb 0x00557B40)  exact     ?CheckEnclSph@m3dSPHERE@@UAEHPAV1@@Z
//   m3dSPHERE::Calc                              v12 0x0056FD70 (feb 0x00557C60)  vtable    ?Calc@m3dSPHERE@@UAEXPAUm3dV@@H@Z
//   m3dSPHERE::Expand                            v12 0x0056FF70 (feb 0x00557E60)  vtable    ?Expand@m3dSPHERE@@UAEXM@Z
//   m3dSPHERE::QualPlane                         v12 0x0056FF80 (feb 0x00557E70)  exact     ?QualPlane@m3dSPHERE@@UAEHPAUm3dV@@0@Z

// ---- m3d_poly.obj ----
//   m3dGetPolyEdge                               v12 0x00570010 (feb 0x00557F00)  exact     ?m3dGetPolyEdge@@YIXPAVm3dPOLY@@HPAUm3dV@@1@Z
//   m3xzPoly3DToPoly2D                           v12 0x00570060 (feb 0x00557F50)  exact     ?m3xzPoly3DToPoly2D@@YIXPAVm3dPOLY@@PAVm2dPOLY@@@Z
//   m3dGetPolyNormal                             v12 0x005700A0 (feb 0x00557F90)  exact     ?m3dGetPolyNormal@@YIXPAVm3dPOLY@@PAUm3dV@@@Z
//   _m3dGetPolyNormal                            v12 0x00570110 (feb 0x00558000)  exact     ?_m3dGetPolyNormal@@YIHPAVm3dPOLY@@PAUm3dV@@@Z
//   m3dGetPlaneNormal                            v12 0x00570180 (feb 0x00558070)  exact     ?m3dGetPlaneNormal@@YIXPAUm3dV@@000@Z
//   m3dGetPlanePerpVector                        v12 0x00570200 (feb 0x005580F0)  exact     ?m3dGetPlanePerpVector@@YAXPAUm3dV@@000@Z
//   m3dCalcTriArea2                              v12 0x005703A0 (feb 0x00558290)  exact     ?m3dCalcTriArea2@@YIMPAUm3dV@@00@Z
//   m3dCalcPolyArea                              v12 0x00570420 (feb 0x00558310)  callgraph ?m3dCalcPolyArea@@YIMPAVm3dPOLY@@@Z
//   m2dReOrientPoly                              v12 0x00570570 (feb 0x00558460)  exact     ?m2dReOrientPoly@@YIXPAVm2dPOLY@@@Z
//   m3dReOrientPoly                              v12 0x00570610 (feb 0x00558500)  exact     ?m3dReOrientPoly@@YIXPAVm3dPOLY@@@Z
//   m2dIsPolyConvexCCW                           v12 0x005706E0 (feb 0x005585D0)  exact     ?m2dIsPolyConvexCCW@@YIHPAVm2dPOLY@@@Z
//   m3dIsectRayPlane                             v12 0x00570750 (feb 0x00558640)  exact     ?m3dIsectRayPlane@@YIHPAUm3dV@@0PAVm3dPOLY@@00PAM@Z
//   m3dIsectRayVNPlane                           v12 0x005708C0 (feb 0x005587B0)  exact     ?m3dIsectRayVNPlane@@YIHPAUm3dV@@0000PAM@Z
//   _m3dIsectRayVNPlane                          v12 0x00570990 (feb 0x00558880)  exact     ?_m3dIsectRayVNPlane@@YIHPAUm3dV@@0000PAM@Z
//   m3dDistPointPoly                             v12 0x00570A90 (feb 0x00558980)  callgraph ?m3dDistPointPoly@@YIHPAUm3dV@@PAVm3dPOLY@@0H0PAMM@Z
//   m3dDistLSegPoly                              v12 0x00571520 (feb 0x00559420)  exact     ?m3dDistLSegPoly@@YIHPAUm3dV@@0PAVm3dPOLY@@000PAMPAHM@Z
//   m3dDistLSegPlane                             v12 0x005717D0 (feb 0x005596D0)  exact     ?m3dDistLSegPlane@@YIHPAUm3dV@@00000PAMM@Z
//   m3dIsectLSegPlane                            v12 0x00571A20 (feb 0x00559920)  callgraph ?m3dIsectLSegPlane@@YIHPAUm3dV@@0000@Z
//   m3dIsectLSegPoly                             v12 0x00571D30 (feb 0x00559C30)  callgraph ?m3dIsectLSegPoly@@YIHPAUm3dV@@0PAVm3dPOLY@@00@Z
//   m2dTriApplyBasisCoord                        v12 0x00572100 (feb 0x0055A000)  exact     ?m2dTriApplyBasisCoord@@YIXPAUm2dTRI@@MMPAUm2dV@@@Z
//   m3dPolyApplyBasisCoord                       v12 0x00572160 (feb 0x0055A060)  exact     ?m3dPolyApplyBasisCoord@@YIXPAVm3dPOLY@@MMPAUm3dV@@@Z
//   m2dIsCrossLSegPolyBoundary_A                 v12 0x005721F0 (feb 0x0055A0F0)  exact     ?m2dIsCrossLSegPolyBoundary_A@@YIHPAVm2dPOLY@@PAUm2dV@@1M@Z

// ---- m3d_lerp.obj ----
//   m3dClampVector_L                             v12 ?????????? (feb 0x0055A140)  -         ?m3dClampVector_L@@YIHPAUm3dV@@0M0@Z
//   m3dClampVector_L                             v12 0x00572900 (feb 0x0055A320)  callgraph ?m3dClampVector_L@@YIHPAUm3dV@@0M@Z
//   m3dClampVector_A                             v12 ?????????? (feb 0x0055A510)  -         ?m3dClampVector_A@@YIHPAUm3dV@@PBU1@M00H@Z
//   m3dLerpVector_L                              v12 0x005731A0 (feb 0x0055ABD0)  exact     ?m3dLerpVector_L@@YIXPAUm3dV@@0MMM0@Z
//   m3dLerpRectCorners                           v12 0x005732F0 (feb 0x0055AD20)  exact     ?m3dLerpRectCorners@@YIMPAUm2dV@@00MMMM@Z

// ---- cgm_phys.obj ----
//   cgmGetThrowSpeed_XZ                          v12 ?????????? (feb 0x0055B1C0)  -         ?cgmGetThrowSpeed_XZ@@YIMPAUm3dV@@0MM0@Z
//   cgmGetThrowSpeed_H                           v12 ?????????? (feb 0x0055B300)  -         ?cgmGetThrowSpeed_H@@YIMPAUm3dV@@0MM0@Z

// ---- cgm_wed.obj ----
//   cgmWED::Init                                 v12 0x00573B40 (feb 0x0055B570)  callgraph ?Init@cgmWED@@UAEHPAVobjOBJ@@@Z
//   cgmWED::MeshGetNFace                         v12 0x00573B50 (feb 0x0055B580)  vtable    ?MeshGetNFace@cgmWED@@MAEHXZ
//   cgmWED::MeshGetNVert                         v12 0x00573B60 (feb 0x0055B590)  vtable    ?MeshGetNVert@cgmWED@@MAEHXZ
//   cgmWED::MeshGetFaceNVert                     v12 0x00573B70 (feb 0x0055B5A0)  vtable    ?MeshGetFaceNVert@cgmWED@@MAEHH@Z
//   cgmWED::MeshGetFaceVertInd                   v12 0x00573B80 (feb 0x0055B5B0)  exact     ?MeshGetFaceVertInd@cgmWED@@MAEHHH@Z
//   cgmWED::MeshGetName                          v12 0x00573BD0 (feb 0x0055B600)  vtable    ?MeshGetName@cgmWED@@MAEPADXZ
//   ?0cgmWED_SYS                                 v12 0x00573BE0 (feb 0x0055B610)  callgraph ??0cgmWED_SYS@@QAE@XZ
//   cgmWED_SYS::Init                             v12 0x00573C30 (feb 0x0055B660)  exact     ?Init@cgmWED_SYS@@UAEHXZ
//   cgmWED_SYS::Term                             v12 0x00573F60 (feb 0x0055B990)  exact     ?Term@cgmWED_SYS@@UAEXXZ
//   cgmWED_NODE::IncidentNode                    v12 0x00574000 (feb 0x0055BA30)  callgraph ?IncidentNode@cgmWED_NODE@@QAEPAV1@PAVcgmWED_EDGE@@@Z
//   cgmWED_NODE::IsIncidentNode                  v12 0x00574020 (feb 0x0055BA50)  exact     ?IsIncidentNode@cgmWED_NODE@@QAEPAVcgmWED_EDGE@@PAV1@@Z
//   cgmWED_SYS::Realloc                          v12 0x00574080 (feb 0x0055BAB0)  exact     ?Realloc@cgmWED_SYS@@AAEHHHH@Z
//   cgmWED_SYS::AddFaceEdge                      v12 0x005742D0 (feb 0x0055BD00)  exact     ?AddFaceEdge@cgmWED_SYS@@AAEHPAVcgmWED_FACE@@HH@Z
//   cgmWED_SYS::ClearState                       v12 0x005743F0 (feb 0x0055BE20)  exact     ?ClearState@cgmWED_SYS@@AAEXXZ
//   cgmWED_3D::CalcDist                          v12 0x00574480 (feb 0x0055BEB0)  vtable    ?CalcDist@cgmWED_3D@@UAEMPAVcgmWED_EDGE@@@Z

// ---- m3d_noise.obj ----
//   m3dInitNoise                                 v12 0x00574570 (feb 0x0055BFB0)  exact     ?m3dInitNoise@@YIHXZ
//   m3dTermNoise                                 v12 0x00574660 (feb 0x0055C0A0)  exact     ?m3dTermNoise@@YIXXZ
//   m3dNOISE::Fill                               v12 0x00574690 (feb 0x0055C0D0)  exact     ?Fill@m3dNOISE@@UAEHH@Z
//   m3dNOISE_GRAD::Fill                          v12 0x00574770 (feb 0x0055C1B0)  vtable    ?Fill@m3dNOISE_GRAD@@UAEHH@Z
//   m3dNOISE_GRAD::GetValue                      v12 0x005748C0 (feb 0x0055C300)  exact     ?GetValue@m3dNOISE_GRAD@@UAEMPAUm3dV@@@Z

// ---- m3d_line.obj ----
//   m2dIsectLSegLSeg                             v12 0x00574B80 (feb 0x0055C5C0)  exact     ?m2dIsectLSegLSeg@@YIHPAUm2dV@@0000@Z
//   m3xzDistPointLSeg_2                          v12 0x00574D50 (feb 0x0055C790)  exact     ?m3xzDistPointLSeg_2@@YIMPAUm3dV@@000@Z
//   m3dDistPointLine                             v12 ?????????? (feb 0x0055C900)  -         ?m3dDistPointLine@@YIMPAUm3dV@@00@Z
//   m3dDistPointLSeg                             v12 0x005750C0 (feb 0x0055CB00)  callgraph ?m3dDistPointLSeg@@YIHPAUm3dV@@00MPAM01@Z
//   m3dDistLSegLSeg                              v12 0x005754A0 (feb 0x0055CEF0)  callgraph ?m3dDistLSegLSeg@@YIHPAUm3dV@@000MPAM00@Z
//   m3dIsIsectYLinePlane                         v12 0x00575C40 (feb 0x0055D690)  exact     ?m3dIsIsectYLinePlane@@YIHPAUm3dV@@000@Z

// ---- m3d_prsm.obj ----
//   m3dPRISM::IsBelongPoint                      v12 0x00575D00 (feb 0x0055D750)  exact     ?IsBelongPoint@m3dPRISM@@UAEHPAUm3dV@@@Z
//   m3dPRISM::IsIsectCyl                         v12 0x00575D70 (feb 0x0055D7C0)  exact     ?IsIsectCyl@m3dPRISM@@UAEHPAUm3dV@@0MM@Z
//   m3dPRISM::IsectRay                           v12 0x00575E40 (feb 0x0055D890)  vtable    ?IsectRay@m3dPRISM@@UAEHPAUm3dV@@0M0PAMPAH@Z
//   m3dPRISM::DistPoint                          v12 0x00576010 (feb 0x0055DA70)  exact     ?DistPoint@m3dPRISM@@UAEHPAUm3dV@@M0PAMPAH@Z
//   m3dPRISM::Check                              v12 0x00576150 (feb 0x0055DBB0)  vtable    ?Check@m3dPRISM@@UAEHXZ
//   m3dPRISM::CalcEnclSph                        v12 0x00576C30 (feb 0x0055E6B0)  vtable    ?CalcEnclSph@m3dPRISM@@UAEXPAVm3dSPHERE@@@Z
//   m3dPRISM::CheckEnclSph                       v12 0x00576D30 (feb 0x0055E7B0)  vtable    ?CheckEnclSph@m3dPRISM@@UAEHPAVm3dSPHERE@@@Z
//   m3dPRISM::GetCenter                          v12 0x00576DC0 (feb 0x0055E840)  exact     ?GetCenter@m3dPRISM@@UAEXPAUm3dV@@@Z
//   m3dPRISM::GetFace                            v12 0x00576E20 (feb 0x0055E8A0)  vtable    ?GetFace@m3dPRISM@@UAEXHPAVm3dPOLY@@PAUm3dV@@@Z
//   m3dPRISM::GetVertices                        v12 0x005770C0 (feb 0x0055EB40)  exact     ?GetVertices@m3dPRISM@@QAEXQAUm3dV@@@Z
//   m3dPRISM::Expand                             v12 0x00577110 (feb 0x0055EB90)  callgraph ?Expand@m3dPRISM@@QAEHPAUm3dV@@0@Z
//   m3dPRISM::Expand                             v12 0x00577740 (feb 0x0055F1D0)  exact     ?Expand@m3dPRISM@@UAEXM@Z
//   m3dPRISM::Calc                               v12 0x00577770 (feb 0x0055F200)  exact     ?Calc@m3dPRISM@@UAEXPAUm3dV@@H@Z
//   m3dPRISM::_MakeBPrismRect                    v12 0x005778B0 (feb 0x0055F340)  exact     ?_MakeBPrismRect@m3dPRISM@@AAEMPAUm3dV@@HMPAM111@Z
//   m3dPRISM::_MakeBPrismPolyBase                v12 0x00577AF0 (feb 0x0055F580)  exact     ?_MakeBPrismPolyBase@m3dPRISM@@AAEXMMMMM@Z

// ---- m3d_obb.obj ----
//   m3dOBB::IsBelongPoint                        v12 0x00578190 (feb 0x0055FC20)  exact     ?IsBelongPoint@m3dOBB@@UAEHPAUm3dV@@@Z
//   m3dOBB::IsIsectCyl                           v12 0x00578350 (feb 0x0055FDE0)  exact     ?IsIsectCyl@m3dOBB@@UAEHPAUm3dV@@0MM@Z
//   m3dOBB::IsectRay                             v12 0x00578660 (feb 0x005600F0)  vtable    ?IsectRay@m3dOBB@@UAEHPAUm3dV@@0M0PAMPAH@Z
//   m3dOBB::DistPoint                            v12 0x00578B00 (feb 0x00560590)  exact     ?DistPoint@m3dOBB@@UAEHPAUm3dV@@M0PAMPAH@Z
//   m3dOBB::GetMatrW2L                           v12 0x00579140 (feb 0x005608E0)  callgraph ?GetMatrW2L@m3dOBB@@QAEXPAVm3dMATR@@0PAVm3dBOX@@@Z
//   m3dOBB::CalcEnclSph                          v12 0x00579440 (feb 0x00560BF0)  vtable    ?CalcEnclSph@m3dOBB@@UAEXPAVm3dSPHERE@@@Z
//   m3dOBB::CheckEnclSph                         v12 0x00579530 (feb 0x00560CE0)  exact     ?CheckEnclSph@m3dOBB@@UAEHPAVm3dSPHERE@@@Z
//   m3dOBB::GetCenter                            v12 0x005796B0 (feb 0x00560E60)  exact     ?GetCenter@m3dOBB@@UAEXPAUm3dV@@@Z
//   m3dOBB::GetFace                              v12 0x00579740 (feb 0x00560EF0)  vtable    ?GetFace@m3dOBB@@UAEXHPAVm3dPOLY@@PAUm3dV@@@Z
//   m3dOBB::GetVertices                          v12 0x00579BB0 (feb 0x00561270)  exact     ?GetVertices@m3dOBB@@QAEXQAUm3dV@@@Z
//   m3dOBB::Expand                               v12 0x00579C90 (feb 0x00561370)  exact     ?Expand@m3dOBB@@UAEXM@Z

// ---- cam_trans.obj ----
//   camCAMERA::ProjPoly                          v12 0x00579E10 (feb 0x005614F0)  exact     ?ProjPoly@camCAMERA@@QAEXPAVm3dPOLY@@PAVm2dPOLY@@@Z
//   camCAMERA::ProjPoint                         v12 0x00579E70 (feb 0x00561550)  exact     ?ProjPoint@camCAMERA@@QAEXPAUm3dV@@PAUm2dV@@@Z
//   camCAMERA::BackProjPoly                      v12 0x00579EA0 (feb 0x00561580)  exact     ?BackProjPoly@camCAMERA@@QAEHPAVm2dPOLY@@PAVm3dPOLY@@1PAUm3dV@@H@Z
//   camCAMERA::ProjPolyOrtho                     v12 0x00579F80 (feb 0x00561660)  exact     ?ProjPolyOrtho@camCAMERA@@SIXPAVm3dPOLY@@PAVm2dPOLY@@@Z
//   camCAMERA::BackProjPolyOrtho                 v12 0x00579FC0 (feb 0x005616A0)  exact     ?BackProjPolyOrtho@camCAMERA@@SIHPAVm2dPOLY@@PAVm3dPOLY@@1PAUm3dV@@H@Z

// ---- m3d_clip.obj ----
//   m2dQualPointLine_A                           v12 0x0057A0A0 (feb 0x00561780)  exact     ?m2dQualPointLine_A@@YIHPAUm2dV@@00M@Z
//   m2dIsBelongPointTri_A                        v12 0x0057A100 (feb 0x005617E0)  exact     ?m2dIsBelongPointTri_A@@YIHPAUm2dTRI@@PAUm2dV@@M@Z
//   m2dIsBelongPointPoly_A                       v12 0x0057A160 (feb 0x00561840)  exact     ?m2dIsBelongPointPoly_A@@YIHPAVm2dPOLY@@PAUm2dV@@M@Z
//   m3xzIsBelongPointPoly_A                      v12 0x0057A300 (feb 0x005619E0)  exact     ?m3xzIsBelongPointPoly_A@@YIHPAVm3dPOLY@@PAUm3dV@@M@Z
//   m3xyIsBelongPointPoly_A                      v12 0x0057A4A0 (feb 0x00561B80)  exact     ?m3xyIsBelongPointPoly_A@@YIHPAVm3dPOLY@@PAUm3dV@@M@Z
//   m3yzIsBelongPointPoly_A                      v12 0x0057A660 (feb 0x00561D40)  exact     ?m3yzIsBelongPointPoly_A@@YIHPAVm3dPOLY@@PAUm3dV@@M@Z
//   m3dIsBelongPointPoly_A                       v12 0x0057A810 (feb 0x00561EF0)  exact     ?m3dIsBelongPointPoly_A@@YIHPAVm3dPOLY@@PAUm3dV@@1M@Z
//   m3dClipPolyNearPlane                         v12 0x0057A900 (feb 0x00561FE0)  exact     ?m3dClipPolyNearPlane@@YIHPAVm3dPOLY@@M0@Z
//   m3dClipPolyPlane                             v12 0x0057AD60 (feb 0x00562440)  exact     ?m3dClipPolyPlane@@YIHPAVm3dPOLY@@PAUm3dV@@10@Z
//   m2dClipPolyRect                              v12 0x0057B020 (feb 0x00562700)  exact     ?m2dClipPolyRect@@YIHPAVm2dPOLY@@00@Z
//   m3xyClipLSegRect                             v12 0x0057BEC0 (feb 0x005635A0)  exact     ?m3xyClipLSegRect@@YIHPAUm3dV@@0PAVm2dPOLY@@00@Z
//   m3dClipLSegNearFar                           v12 0x0057C330 (feb 0x00563A10)  exact     ?m3dClipLSegNearFar@@YIHPAUm3dV@@0MM@Z

// ---- m2d_matr.obj ----
//   m3dInit2DMatr                                v12 0x0057C420 (feb 0x00563B00)  exact     ?m3dInit2DMatr@@YIHXZ
//   m2dMATR::Identity                            v12 0x0057C470 (feb 0x00563B50)  exact     ?Identity@m2dMATR@@QAEXXZ
//   m2dMATR::Scale                               v12 0x0057C4A0 (feb 0x00563B80)  exact     ?Scale@m2dMATR@@QAEXMMH@Z
//   m2dMATR::Translate                           v12 0x0057C510 (feb 0x00563BF0)  exact     ?Translate@m2dMATR@@QAEXMMH@Z
//   m2dMATR::Rotate                              v12 0x0057C580 (feb 0x00563C60)  exact     ?Rotate@m2dMATR@@QAEXMH@Z
//   m2dMATR::MakeRot                             v12 0x0057C6C0? (feb 0x00563DA0)  callgraph ?MakeRot@m2dMATR@@QAEXM@Z
//   m2dMATR::MakePolyPoly                        v12 0x0057C700 (feb 0x00563DD0)  exact     ?MakePolyPoly@m2dMATR@@QAEHPAVm2dPOLY@@0@Z

// ---- cam_qual.obj ----
//   camCAMERA::CalcPOVQual                       v12 0x0057CA90 (feb 0x00564160)  callgraph ?CalcPOVQual@camCAMERA@@QAEXXZ
//   camCAMERA::QualBoxPOV                        v12 0x0057CF80 (feb 0x00564650)  exact     ?QualBoxPOV@camCAMERA@@QAEHPAVm3dBOX@@@Z
//   camCAMERA::QualOBBPOV                        v12 0x0057D5D0 (feb 0x00564CA0)  exact     ?QualOBBPOV@camCAMERA@@QAEHPAVm3dOBB@@@Z
//   camCAMERA::QualPrismPOV                      v12 0x0057DE60 (feb 0x00565540)  exact     ?QualPrismPOV@camCAMERA@@QAEHPAVm3dPRISM@@@Z
//   camCAMERA::QualSpherePOV                     v12 0x0057E0E0 (feb 0x005657C0)  callgraph ?QualSpherePOV@camCAMERA@@QAEHPAVm3dSPHERE@@@Z

// ---- cgm_search.obj ----
//   cgmSEARCH::Init                              v12 0x0057E340 (feb 0x00565A20)  exact     ?Init@cgmSEARCH@@UAEHHHPAXPAH@Z
//   cgmSEARCH::Search                            v12 0x0057E3D0 (feb 0x00565AB0)  exact     ?Search@cgmSEARCH@@UAEHPAXH@Z
//   cgmSEARCH::Term                              v12 0x0057E450 (feb 0x00565B30)  exact     ?Term@cgmSEARCH@@UAEXXZ
//   cgmSEARCH_VLIST::Init                        v12 0x0057E480 (feb 0x00565B60)  exact     ?Init@cgmSEARCH_VLIST@@UAEHHHPAXPAH@Z
//   cgmSEARCH_VLIST::Term                        v12 0x0057E5D0 (feb 0x00565CB0)  exact     ?Term@cgmSEARCH_VLIST@@UAEXXZ
//   cgmSEARCH_VLIST::CmpItem                     v12 0x0057E620 (feb 0x00565D00)  exact     ?CmpItem@cgmSEARCH_VLIST@@UAEHPAX0@Z
//   cgmSEARCH_INT_LIST::CmpItem                  v12 0x0057E6A0 (feb 0x00565D80)  exact     ?CmpItem@cgmSEARCH_INT_LIST@@UAEHPAX0@Z
//   cgmSEARCH::QSortCB                           v12 0x0057E700 (feb 0x00565DE0)  exact     ?QSortCB@cgmSEARCH@@SAHPBX0@Z

// ---- cgm_wed_algo.obj ----
//   cgmWED_SYS::DepthFirstSearch                 v12 0x0057E760 (feb 0x00565E40)  exact     ?DepthFirstSearch@cgmWED_SYS@@QAEHPAVcgmQUERY@@@Z
//   cgmWED_SYS::Dijkstra                         v12 0x0057E7E0 (feb 0x00565EC0)  exact     ?Dijkstra@cgmWED_SYS@@QAEXHPAHH0PAMPAVcgmQUERY@@@Z
//   cgmWED_SYS::CalcBoundary                     v12 0x0057E9D0 (feb 0x005660B0)  exact     ?CalcBoundary@cgmWED_SYS@@QAEXH@Z
//   cgmWED_SYS::DepthSearch                      v12 0x0057EAF0 (feb 0x005661D0)  exact     ?DepthSearch@cgmWED_SYS@@AAEXPAVcgmWED_NODE@@HPAVcgmQUERY@@@Z
//   cgmQUERY_BOUNDARY::ProcessEdge               v12 0x0057EB80 (feb 0x00566260)  exact     ?ProcessEdge@cgmQUERY_BOUNDARY@@UAEHPAVcgmWED_NODE@@PAVcgmWED_EDGE@@@Z

// ---- m3d_vox.obj ----
//   voxGRID::GetNVoxels                          v12 0x0057EBC0 (feb 0x005662A0)  callgraph ?GetNVoxels@voxGRID@@QAEHXZ
//   voxGRID::GetPosInd                           v12 0x0057EBD0 (feb 0x005662B0)  exact     ?GetPosInd@voxGRID@@QAEXPAUm3dV@@PAH11H@Z
//   voxGRID::ClampIndices                        v12 0x0057ECB0 (feb 0x00566390)  exact     ?ClampIndices@voxGRID@@QAEXPAH00@Z
//   voxGRID::CalcVoxSize                         v12 0x0057ED20 (feb 0x00566400)  exact     ?CalcVoxSize@voxGRID@@QAEHXZ
//   voxCalcNVoxel                                v12 0x0057EE60 (feb 0x00566540)  callgraph ?voxCalcNVoxel@@YIHMM@Z
//   voxGRID::CalcVoxBBox                         v12 0x0057EE80 (feb 0x00566560)  exact     ?CalcVoxBBox@voxGRID@@QAEXHHHPAVm3dBOX@@@Z
//   voxGRID::MakeIndListRay                      v12 0x0057EED0 (feb 0x005665B0)  exact     ?MakeIndListRay@voxGRID@@QAEXPAUm3dV@@0PAH1H@Z

// ---- m3d_quat.obj ----
//   m3dIsEqualQuat_A                             v12 0x0057F720 (feb 0x00566E00)  exact     ?m3dIsEqualQuat_A@@YIHPAVm3dQUAT@@0M@Z
//   m3dSlerpQuat                                 v12 0x0057F830 (feb 0x00566F10)  exact     ?m3dSlerpQuat@@YIXPAVm3dQUAT@@0M0@Z
//   m3dQUAT::Identity                            v12 (inlined)   (feb 0x00567160)  gone      ?Identity@m3dQUAT@@QAEXXZ
//   m3dQUAT::Invert                              v12 0x0057FA80 (feb 0x00567180)  exact     ?Invert@m3dQUAT@@QAEXXZ
//   m3dQUAT::SetAngAxis                          v12 0x0057FAB0 (feb 0x005671B0)  exact     ?SetAngAxis@m3dQUAT@@QAEXMPAUm3dV@@@Z
//   m3dQUAT::GetMatr                             v12 0x0057FAF0 (feb 0x005671F0)  exact     ?GetMatr@m3dQUAT@@QAEXPAVm3dMATR@@@Z
//   m3dQUAT::SetRotMatr                          v12 0x0057FC20 (feb 0x00567320)  callgraph ?SetRotMatr@m3dQUAT@@QAEXPAVm3dMATR@@@Z
//   m3dQUAT::SetMatr                             v12 0x0057FEA0 (feb 0x005675B0)  exact     ?SetMatr@m3dQUAT@@QAEXPAVm3dMATR@@@Z

// ---- cam_qual.obj ----
//   _camSeparateBBoxPOV                          v12 0x0057D210 (feb 0x005648E0)  callgraph ?_camSeparateBBoxPOV@@YIHPAUcamPOV_QUAL@@PAUm3dV@@1QAU2@12M@Z
//   _camSeparateOBBPOV                           v12 0x0057D810 (feb 0x00564EE0)  callgraph ?_camSeparateOBBPOV@@YIHPAUcamPOV_QUAL@@PAUm3dV@@1QAU2@12M@Z
//   _camSeparateOBBPOV_AxisPOV                   v12 0x0057DC40 (feb 0x00565310)  callgraph ?_camSeparateOBBPOV_AxisPOV@@YIHPAUcamPOV_QUAL@@HPAUm3dV@@QAU2@@Z

// ---- m3d_clip.obj ----
//   _m2dClipPolyRect                             v12 0x0057B160 (feb 0x00562840)  exact     ?_m2dClipPolyRect@@YIHPAVm2dPOLY@@00HH@Z

// ---- m3d_noise.obj ----
//   _noiseFillHash                               v12 0x00574AB0 (feb 0x0055C4F0)  exact     ?_noiseFillHash@@YIHPAHH@Z

// ---- cam.obj ----
//   _camMatrInvert                               v12 0x0056C480 (feb 0x005541B0)  exact     ?_camMatrInvert@@YIXPAVm3dMATR@@@Z

// ---- m3d.obj ----
//   _m3dDetectSIMD                               v12 0x00569760 (feb 0x005516A0)  exact     ?_m3dDetectSIMD@@YIXXZ

// ---- m3d_matr.obj ----
//   _m2dCalcMatrDet                              v12 0x00564A00 (feb 0x0054D4E0)  exact     ?_m2dCalcMatrDet@@YIMPAVm3dMATR@@FFFFFF@Z
