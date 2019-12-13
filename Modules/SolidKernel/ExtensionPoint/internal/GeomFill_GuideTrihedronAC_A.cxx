/*!
* \file    GeomFill_GuideTrihedronAC_A.cxx
*
* \brief   This file is a full copy of GeomFill_GuideTrihedronAC.cxx from OpenCASCADE, but with support for 'subCurveLaws'.
*/
 
// Created by: Stephanie HUMEAU
// Copyright (c) 1998-1999 Matra Datavision
// Copyright (c) 1999-2014 OPEN CASCADE SAS
//
// This file is part of Open CASCADE Technology software library.
//
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License version 2.1 as published
// by the Free Software Foundation, with special exception defined in the file
// OCCT_LGPL_EXCEPTION.txt. Consult the file LICENSE_LGPL_21.txt included in OCCT
// distribution for complete text of the license and disclaimer of any warranty.
//
// Alternatively, this file may be used under the terms of Open CASCADE
// commercial license or contractual agreement.

// Creted:	Tue Jun 23 15:39:24 1998

#include <GeomFill_GuideTrihedronAC.hxx>
#include <internal/GeomFill_GuideTrihedronAC_A.hxx>

#include <GCPnts_AbscissaPoint.hxx>

#include <gp_Pnt.hxx>
#include <gp_Dir.hxx>
#include <gp_Vec.hxx>
#include <Precision.hxx>
#include <TColStd_SequenceOfReal.hxx>

#include <Approx_CurvlinFunc.hxx>
#include <Adaptor3d_Curve.hxx>
#include <GeomAdaptor.hxx>
#include <GeomAdaptor_HCurve.hxx>
#include <TColStd_Array1OfReal.hxx>

#include <GeomFill_Frenet.hxx>
#include <GeomLib.hxx>

#include <algorithm>


IMPLEMENT_STANDARD_RTTIEXT(GeomFill_GuideTrihedronAC_A, GeomFill_TrihedronWithGuide)

//=======================================================================
//function : GuideTrihedron
//purpose  : Constructor
//=======================================================================
GeomFill_GuideTrihedronAC_A::GeomFill_GuideTrihedronAC_A(const Handle(Adaptor3d_HCurve) & guide)
{
  myCurve.Nullify();
  myGuide =  guide;
  myTrimG =  guide;
  myGuideAC = new (Approx_CurvlinFunc) (myGuide,1.e-7);
  Lguide = myGuideAC->GetLength(); 
  UTol = STol = Precision::PConfusion();
  Orig1 = 0; // origines pour le cas path multi-edges
  Orig2 = 1;
}

//=======================================================================
//function : Guide
//purpose  : calculation of trihedron
//=======================================================================

 Handle(Adaptor3d_HCurve) GeomFill_GuideTrihedronAC_A::Guide()const
{
  return myGuide;
}

//=======================================================================
//function : D0
//purpose  : calculation of trihedron
//=======================================================================
 Standard_Boolean GeomFill_GuideTrihedronAC_A::D0(const Standard_Real Param,
						gp_Vec& Tangent,
						gp_Vec& Normal,
						gp_Vec& BiNormal) 
{ 
     if (_subCurveParameters.size() > 0) {
         int lawIndex = static_cast<int>(std::distance(_subCurveParameters.begin(), std::lower_bound(_subCurveParameters.begin(), _subCurveParameters.end(), Param)));
         lawIndex = std::min(lawIndex, static_cast<int>(_subCurveParameters.size()) - 1);
         double parameter = std::min(Param, *_subCurveParameters.rbegin());
         return _subCurveLaws[lawIndex]->D0(parameter, Tangent, Normal, BiNormal);
     }
     else {
         Standard_Real s = myCurveAC->GetSParameter(Param); // abscisse curviligne <=> Param
         Standard_Real OrigG = Orig1 + s*(Orig2 - Orig1); // abscisse curv sur le guide (cas multi-edges)
         Standard_Real tG = myGuideAC->GetUParameter(myGuide->GetCurve(), OrigG, 1); // param <=> s sur theGuide

         gp_Pnt P, PG;
         gp_Vec To, B;
         myTrimmed->D1(Param, P, To);//point et derivee au parametre Param sur myCurve
         myTrimG->D0(tG, PG);// point au parametre tG sur myGuide
         myCurPointOnGuide = PG;

         gp_Vec n(P, PG); // vecteur definissant la normale

         Normal = n.Normalized();
         B = To.Crossed(Normal);
         BiNormal = B / B.Magnitude();
         Tangent = Normal.Crossed(BiNormal);
         Tangent.Normalize();

         return Standard_True;
     }
}

//=======================================================================
//function : D1
//purpose  : calculation of trihedron and first derivative
//=======================================================================
 Standard_Boolean GeomFill_GuideTrihedronAC_A::D1(const Standard_Real Param,
						gp_Vec& Tangent,
						gp_Vec& DTangent,
						gp_Vec& Normal,
						gp_Vec& DNormal,
						gp_Vec& BiNormal,	   
						gp_Vec& DBiNormal) 
{ 
     if (_subCurveParameters.size() > 0) {
         int lawIndex = static_cast<int>(std::distance(_subCurveParameters.begin(), std::lower_bound(_subCurveParameters.begin(), _subCurveParameters.end(), Param)));
         lawIndex = std::min(lawIndex, static_cast<int>(_subCurveParameters.size()) - 1);
         double parameter = std::min(Param, *_subCurveParameters.rbegin());
         return _subCurveLaws[lawIndex]->D1(parameter, Tangent, DTangent, Normal, DNormal, BiNormal, DBiNormal);
     }
     else {
         //triedre
         Standard_Real s, OrigG, tG, dtg;
         // abscisse curviligne <=> Param
         s = myCurveAC->GetSParameter(Param);
         // parametre <=> s sur theGuide
         OrigG = Orig1 + s*(Orig2 - Orig1);
         // parametre <=> s sur  theGuide
         tG = myGuideAC->GetUParameter(myGuide->GetCurve(), OrigG, 1);

         gp_Pnt P, PG;
         gp_Vec To, DTo, TG, B, BPrim;

         myTrimmed->D2(Param, P, To, DTo);
         myTrimG->D1(tG, PG, TG);
         myCurPointOnGuide = PG;

         gp_Vec n(P, PG), dn;
         Standard_Real Norm = n.Magnitude();
         if (Norm < 1.e-12) {
             Norm = 1;
#ifdef OCCT_DEBUG
             cout << "GuideTrihedronAC : Normal indefinie" << endl;
#endif
         }

         n /= Norm;
         //derivee de n par rapport a Param
         dtg = (Orig2 - Orig1)*(To.Magnitude() / TG.Magnitude())*(Lguide / L);
         dn.SetLinearForm(dtg, TG, -1, To);
         dn /= Norm;

         // triedre
         Normal = n;
         B = To.Crossed(Normal);
         Standard_Real NormB = B.Magnitude();
         B /= NormB;

         BiNormal = B;

         Tangent = Normal.Crossed(BiNormal);
         Tangent.Normalize();

         // derivee premiere
         DNormal.SetLinearForm(-(n.Dot(dn)), n, dn);

         BPrim.SetLinearForm(DTo.Crossed(Normal), To.Crossed(DNormal));

         DBiNormal.SetLinearForm(-(B.Dot(BPrim)), B, BPrim);
         DBiNormal /= NormB;

         DTangent.SetLinearForm(Normal.Crossed(DBiNormal), DNormal.Crossed(BiNormal));

         return Standard_True;
     }
}


//=======================================================================
//function : D2
//purpose  : calculation of trihedron and derivatives
//=======================================================================
 Standard_Boolean GeomFill_GuideTrihedronAC_A::D2(const Standard_Real Param,
						gp_Vec& Tangent,
						gp_Vec& DTangent,
						gp_Vec& D2Tangent,
						gp_Vec& Normal,
						gp_Vec& DNormal,
						gp_Vec& D2Normal,
						gp_Vec& BiNormal,			  
						gp_Vec& DBiNormal,		  
						gp_Vec& D2BiNormal) 
{ 
     if (_subCurveParameters.size() > 0) {
         int lawIndex = static_cast<int>(std::distance(_subCurveParameters.begin(), std::lower_bound(_subCurveParameters.begin(), _subCurveParameters.end(), Param)));
         lawIndex = std::min(lawIndex, static_cast<int>(_subCurveParameters.size()) - 1);
         double parameter = std::min(Param, *_subCurveParameters.rbegin());
         return _subCurveLaws[lawIndex]->D2(parameter, Tangent, DTangent, D2Tangent, Normal, DNormal, D2Normal, BiNormal, DBiNormal, D2BiNormal);
     }
     else {
         // abscisse curviligne <=> Param
         Standard_Real s = myCurveAC->GetSParameter(Param);
         // parametre <=> s sur theGuide
         Standard_Real OrigG = Orig1 + s*(Orig2 - Orig1);
         Standard_Real tG = myGuideAC->GetUParameter(myGuide->GetCurve(),
             OrigG, 1);

         gp_Pnt P, PG;
         gp_Vec TG, DTG;
         //  gp_Vec To,DTo,D2To,B;
         gp_Vec To, DTo, D2To;

         myTrimmed->D3(Param, P, To, DTo, D2To);
         myTrimG->D2(tG, PG, TG, DTG);
         myCurPointOnGuide = PG;

         Standard_Real NTo = To.Magnitude();
         Standard_Real N2To = To.SquareMagnitude();
         Standard_Real NTG = TG.Magnitude();
         Standard_Real N2Tp = TG.SquareMagnitude();
         Standard_Real d2tp_dt2, dtg_dt;
         dtg_dt = (Orig2 - Orig1)*(NTo / NTG)*(Lguide / L);

         gp_Vec n(P, PG); // vecteur definissant la normale
         Standard_Real Norm = n.Magnitude(), ndn;
         //derivee de n par rapport a Param
         gp_Vec dn, d2n;
         dn.SetLinearForm(dtg_dt, TG, -1, To);

         //derivee seconde de tG par rapport a Param
         d2tp_dt2 = (Orig2 - Orig1)*(Lguide / L) *
             (DTo.Dot(To) / (NTo*NTG) - N2To*TG*DTG*(Lguide / L) / (N2Tp*N2Tp));
         //derivee seconde de n par rapport a Param
         d2n.SetLinearForm(dtg_dt*dtg_dt, DTG, d2tp_dt2, TG, -1, DTo);

         if (Norm > 1.e-9) {
             n /= Norm;
             dn /= Norm;
             d2n /= Norm;
         }
         //triedre
         Normal = n;

         gp_Vec TN, DTN, D2TN;
         TN = To.Crossed(Normal);


         Standard_Real Norma = TN.Magnitude();
         if (Norma > 1.e-9) TN /= Norma;

         BiNormal = TN;

         Tangent = Normal.Crossed(BiNormal);
         //  Tangent.Normalize();

         // derivee premiere du triedre
         //  gp_Vec DTN = DTo.Crossed(Normal);
         //  gp_Vec TDN = To.Crossed(DNormal);
         //  gp_Vec DT = DTN + TDN;

         ndn = n.Dot(dn);
         DNormal.SetLinearForm(-ndn, n, dn);

         DTN.SetLinearForm(DTo.Crossed(Normal), To.Crossed(DNormal));
         DTN /= Norma;
         Standard_Real TNDTN = TN.Dot(DTN);

         DBiNormal.SetLinearForm(-TNDTN, TN, DTN);

         DTangent.SetLinearForm(Normal.Crossed(DBiNormal),
             DNormal.Crossed(BiNormal));


         //derivee seconde du triedre
#ifdef OCCT_DEBUG
         gp_Vec DTDN = DTo.Crossed(DNormal);
#else
         DTo.Crossed(DNormal);
#endif
         Standard_Real TN2 = TN.SquareMagnitude();

         D2Normal.SetLinearForm(-2 * ndn, dn,
             3 * ndn*ndn - (dn.SquareMagnitude() + n.Dot(d2n)), n,
             d2n);


         D2TN.SetLinearForm(1, D2To.Crossed(Normal),
             2, DTo.Crossed(DNormal),
             To.Crossed(D2Normal));
         D2TN /= Norma;

         D2BiNormal.SetLinearForm(-2 * TNDTN, DTN,
             3 * TNDTN*TNDTN - (TN2 + TN.Dot(D2TN)), TN,
             D2TN);

         D2Tangent.SetLinearForm(1, D2Normal.Crossed(BiNormal),
             2, DNormal.Crossed(DBiNormal),
             Normal.Crossed(D2BiNormal));

         //  return Standard_True;
         return Standard_False;
     }

}


//=======================================================================
//function : Copy
//purpose  : 
//=======================================================================
 Handle(GeomFill_TrihedronLaw) GeomFill_GuideTrihedronAC_A::Copy() const
{
 Handle(GeomFill_GuideTrihedronAC_A) copy = 
   new (GeomFill_GuideTrihedronAC_A) (myGuide);
 copy->SetCurve(myCurve);
 copy->Origine(Orig1,Orig2);
 copy->setSubCurveLaws(_subCurveLaws);
 return copy;
} 

//=======================================================================
//function : SetCurve
//purpose  : 
//=======================================================================
 void GeomFill_GuideTrihedronAC_A::SetCurve(const Handle(Adaptor3d_HCurve)& C) 
{
  myCurve = C;
  myTrimmed = C;
  if (!myCurve.IsNull()) {
    myCurveAC = new (Approx_CurvlinFunc) (C,1.e-7);
    L = myCurveAC->GetLength();
//    CorrectOrient(myGuide);
  }
}


//=======================================================================
//function : NbIntervals
//purpose  : 
//=======================================================================
 Standard_Integer GeomFill_GuideTrihedronAC_A::NbIntervals(const GeomAbs_Shape S) const
{
  Standard_Integer Nb;
  Nb = myCurveAC->NbIntervals(S);
  TColStd_Array1OfReal DiscC(1, Nb+1);
  myCurveAC->Intervals(DiscC, S);
  Nb =  myGuideAC->NbIntervals(S);
  TColStd_Array1OfReal DiscG(1, Nb+1);
  myGuideAC->Intervals(DiscG, S);

  TColStd_SequenceOfReal Seq;
  GeomLib::FuseIntervals(DiscC, DiscG, Seq);
  
  return Seq.Length()-1;

}

//======================================================================
//function :Intervals
//purpose  : 
//=======================================================================
 void GeomFill_GuideTrihedronAC_A::Intervals(TColStd_Array1OfReal& TT,
					   const GeomAbs_Shape S) const
{
  Standard_Integer Nb, ii;
  Nb = myCurveAC->NbIntervals(S);
  TColStd_Array1OfReal DiscC(1, Nb+1);
  myCurveAC->Intervals(DiscC, S);
  Nb =  myGuideAC->NbIntervals(S);
  TColStd_Array1OfReal DiscG(1, Nb+1);
  myGuideAC->Intervals(DiscG, S);

  TColStd_SequenceOfReal Seq;
  GeomLib::FuseIntervals(DiscC, DiscG, Seq); 
  Nb = Seq.Length();

  for (ii=1; ii<=Nb; ii++) {
    TT(ii) =  myCurveAC->GetUParameter(myCurve->GetCurve(), Seq(ii), 1);
  }

}

//======================================================================
//function :SetInterval
//purpose  : 
//=======================================================================
void GeomFill_GuideTrihedronAC_A::SetInterval(const Standard_Real First,
					    const Standard_Real Last) 
{
    if (_subCurveParameters.size() > 0) {
        int lawIndexFirst = static_cast<int>(std::distance(_subCurveParameters.begin(), std::lower_bound(_subCurveParameters.begin(), _subCurveParameters.end(), First)));
        int lawIndexLast = static_cast<int>(std::distance(_subCurveParameters.begin(), std::lower_bound(_subCurveParameters.begin(), _subCurveParameters.end(), Last)));
        lawIndexFirst = std::min(lawIndexFirst, static_cast<int>(_subCurveParameters.size()) - 1);
        lawIndexLast = std::min(lawIndexLast, static_cast<int>(_subCurveParameters.size()) - 1);

        double cur = First;
        for (int i = lawIndexFirst; i <= lawIndexLast; ++i) {
            _subCurveLaws[i]->SetInterval(cur, i == lawIndexLast ? std::min(*_subCurveParameters.rbegin(),Last) : _subCurveParameters[i]);
            cur = _subCurveParameters[i];
        }
    }
    else {
        myTrimmed = myCurve->Trim(First, Last, UTol);
        Standard_Real Sf, Sl, U;

        Sf = myCurveAC->GetSParameter(First);
        Sl = myCurveAC->GetSParameter(Last);
        //  if (Sl>1) Sl=1;
        //  myCurveAC->Trim(Sf, Sl, UTol);

        U = Orig1 + Sf*(Orig2 - Orig1);
        Sf = myGuideAC->GetUParameter(myGuide->GetCurve(), U, 1);
        U = Orig1 + Sl*(Orig2 - Orig1);
        Sl = myGuideAC->GetUParameter(myGuide->GetCurve(), U, 1);
        myTrimG = myGuide->Trim(Sf, Sl, UTol);
    }
}



//=======================================================================
//function : GetAverageLaw
//purpose  : 
//=======================================================================
 void GeomFill_GuideTrihedronAC_A::GetAverageLaw(gp_Vec& ATangent,
					       gp_Vec& ANormal,
					       gp_Vec& ABiNormal) 
{
  Standard_Integer ii;
  Standard_Real t, Delta = (myCurve->LastParameter() - 
			    myCurve->FirstParameter())/20.001;

  ATangent.SetCoord(0.,0.,0.);
  ANormal.SetCoord(0.,0.,0.);
  ABiNormal.SetCoord(0.,0.,0.);
  gp_Vec T, N, B;
  
  for (ii=1; ii<=20; ii++) {
    t = myCurve->FirstParameter() +(ii-1)*Delta;
    D0(t, T, N, B);
    ATangent +=T;
    ANormal  +=N;
    ABiNormal+=B;
  }
  ATangent  /= 20;
  ANormal   /= 20;
  ABiNormal /= 20; 
}

//=======================================================================
//function : IsConstant
//purpose  : 
//=======================================================================
 Standard_Boolean GeomFill_GuideTrihedronAC_A::IsConstant() const
{
  return  Standard_False;
}

//=======================================================================
//function : IsOnlyBy3dCurve
//purpose  : 
//=======================================================================
 Standard_Boolean GeomFill_GuideTrihedronAC_A::IsOnlyBy3dCurve() const
{
  return Standard_False;
}

//=======================================================================
//function : Origine
//purpose  : 
//=======================================================================
 void GeomFill_GuideTrihedronAC_A::Origine(const Standard_Real OrACR1,
					 const Standard_Real OrACR2)
{
  Orig1 = OrACR1;
  Orig2 = OrACR2;
}

 void GeomFill_GuideTrihedronAC_A::setSubCurveLaws(const std::vector<Handle(GeomFill_GuideTrihedronAC_A)>& subCurveLaws)
 {
     _subCurveLaws = subCurveLaws;
     _subCurveParameters.clear();

     double curParam = 0;
     for (size_t i = 0; i < subCurveLaws.size(); ++i) {
         curParam = subCurveLaws[i]->myCurve->LastParameter();
         _subCurveParameters.push_back(curParam);
     }
 }
