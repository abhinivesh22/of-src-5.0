/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | Copyright (C) 2011 OpenFOAM Foundation
     \\/     M anipulation  |
-------------------------------------------------------------------------------
License
    This file is part of OpenFOAM.

    OpenFOAM is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    OpenFOAM is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
    for more details.

    You should have received a copy of the GNU General Public License
    along with OpenFOAM.  If not, see <http://www.gnu.org/licenses/>.

\*---------------------------------------------------------------------------*/

#include "modifiedSEMFvPatchField.H"
#include "clock.H"
#include "vectorList.H"
#include "fvPatchFieldMapper.H"
#include "volFields.H"
#include "surfaceFields.H"
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

template<class Type>
modifiedSEMFvPatchField<Type>::modifiedSEMFvPatchField
(
    const fvPatch& p,
    const DimensionedField<Type, volMesh>& iF
)
:
    	fixedValueFvPatchField<Type>(p, iF),
	mapperPtr_(NULL),
        pointsDict_
                        (
                            IOobject
                            (
                                "points",
                                this->db().time().constant(),//timeName(this->db().time().timeOutputValue()-this->db().time().deltaTValue()),
                                this->db(),
                                IOobject::MUST_READ,
                                IOobject::AUTO_WRITE
                            )
                        ),
        statisticsDict_
                        (
                            IOobject
                            (
                                "statistics",
                                this->db().time().constant(),//timeName(this->db().time().timeOutputValue()-this->db().time().deltaTValue()),
                                this->db(),
                                IOobject::MUST_READ,
                                IOobject::AUTO_WRITE
                            )
                        ),


	ranGen_(clock().getTime()),
        n_(100),                        		//number of eddies
        sigmaMin_(0.5,0.5,0.5),         		//min length scale in all direction
        sigmaMax_(0.5,0.5,0.5),     	    	//max length scale in all direction     
        Uinf_(1,0,0),   			//mean velocity
	cf_(1),					//skin friction coeff
        L_(1),          				//eddy box length in flow direction
	referenceField_(p.size()),			//reference filed
        curTimeIndex_(-1),       			// time index 
	pp_(100),
        sigma_(100),
        intensity_(100),
        rndsign_(100),
	delta_(0.001),
	fstu_(0),
	maxy_(0.01)
 

{}


template<class Type>
modifiedSEMFvPatchField<Type>::modifiedSEMFvPatchField
(
    const modifiedSEMFvPatchField<Type>& ptf,
    const fvPatch& p,
    const DimensionedField<Type, volMesh>& iF,
    const fvPatchFieldMapper& mapper
)
:
	fixedValueFvPatchField<Type>(ptf, p, iF, mapper),
	mapperPtr_(NULL),
        pointsDict_(ptf.pointsDict_),
	statisticsDict_(ptf.pointsDict_),
	ranGen_(clock().getTime()),
        n_(ptf.n_),
        sigmaMin_(ptf.sigmaMin_),
        sigmaMax_(ptf.sigmaMax_),
        Uinf_(ptf.Uinf_),
        cf_(ptf.cf_),
        L_(ptf.L_),
	referenceField_(ptf.referenceField_, mapper),
        curTimeIndex_(-1),
        pp_(ptf.pp_),
        sigma_(ptf.sigma_),
        intensity_(ptf.intensity_),
        rndsign_(ptf.rndsign_),
	delta_(ptf.delta_),
	fstu_(ptf.fstu_),
	maxy_(ptf.maxy_)
{}


template<class Type>
modifiedSEMFvPatchField<Type>::modifiedSEMFvPatchField
(
    const fvPatch& p,
    const DimensionedField<Type, volMesh>& iF,
    const dictionary& dict
)
:
    	fixedValueFvPatchField<Type>(p, iF),
        mapperPtr_(NULL),
        pointsDict_
                        (
                            IOobject
                            (
                                "points",
                                this->db().time().constant(),//timeName(this->db().time().timeOutputValue()-this->db().time().deltaTValue()),
                                this->db(),
                                IOobject::MUST_READ,
                                IOobject::AUTO_WRITE
                            )
                        ),
        statisticsDict_
                        (
                            IOobject
                            (
                                "statistics",
                                this->db().time().constant(),//timeName(this->db().time().timeOutputValue()-this->db().time().deltaTValue()),
                                this->db(),
                                IOobject::MUST_READ,
                                IOobject::AUTO_WRITE
                            )
                        ),


	ranGen_(clock().getTime()),
        n_(readScalar(dict.lookup("n"))),
        sigmaMin_(dict.lookup("sigmaMin")),
        sigmaMax_(dict.lookup("sigmaMax")),
        Uinf_(dict.lookup("Uinf")),
        cf_(readScalar(dict.lookup("cf"))),
        L_(readScalar(dict.lookup("L"))),
	referenceField_("referenceField", dict, p.size()),
        curTimeIndex_(-1),
        pp_(vectorList(pointsDict_.lookup("points"))),
        sigma_(vectorList(pointsDict_.lookup("sigma"))),
        intensity_(vectorList(pointsDict_.lookup("intensity"))),
        rndsign_(vectorList(pointsDict_.lookup("rndsign"))),
	delta_(readScalar(dict.lookup("delta"))),
	fstu_(readScalar(dict.lookup("fstu"))),
 	maxy_(readScalar(dict.lookup("maxy")))

{
    if (dict.found("value"))
    {
        fixedValueFvPatchField<Type>::operator==
        (
            Field<Type>("value", dict, p.size())
        );
    }
    else
    {
        fixedValueFvPatchField<Type>::operator==(referenceField_);
    }
}


template<class Type>
modifiedSEMFvPatchField<Type>::modifiedSEMFvPatchField
(
    const modifiedSEMFvPatchField<Type>& ptf
)
:
    	fixedValueFvPatchField<Type>(ptf),
        mapperPtr_(NULL),
	pointsDict_(ptf.pointsDict_),
        statisticsDict_(ptf.pointsDict_),
	ranGen_(ptf.ranGen_),
        n_(ptf.n_),
        sigmaMin_(ptf.sigmaMin_),
        sigmaMax_(ptf.sigmaMax_),
        Uinf_(ptf.Uinf_),
        cf_(ptf.cf_),
        L_(ptf.L_),
	referenceField_(ptf.referenceField_),
        curTimeIndex_(-1),
        pp_(ptf.pp_),
        sigma_(ptf.sigma_),
        intensity_(ptf.intensity_),
        rndsign_(ptf.rndsign_),
	delta_(ptf.delta_),
	fstu_(ptf.fstu_),
 	maxy_(ptf.maxy_)

{}


template<class Type>
modifiedSEMFvPatchField<Type>::modifiedSEMFvPatchField
(
    const modifiedSEMFvPatchField<Type>& ptf,
    const DimensionedField<Type, volMesh>& iF
)
:
    fixedValueFvPatchField<Type>(ptf, iF),
        mapperPtr_(NULL),
	pointsDict_(ptf.pointsDict_),
        statisticsDict_(ptf.pointsDict_),
	ranGen_(ptf.ranGen_),
        n_(ptf.n_),
        sigmaMin_(ptf.sigmaMin_),
        sigmaMax_(ptf.sigmaMax_),
        Uinf_(ptf.Uinf_),
        cf_(ptf.cf_),
        L_(ptf.L_),
        referenceField_(ptf.referenceField_),
        curTimeIndex_(-1),
        pp_(ptf.pp_),
        sigma_(ptf.sigma_),
        intensity_(ptf.intensity_),
        rndsign_(ptf.rndsign_),
	delta_(ptf.delta_),
	fstu_(ptf.fstu_),
	maxy_(ptf.maxy_)

{}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

template<class Type>
void modifiedSEMFvPatchField<Type>::autoMap
(
    const fvPatchFieldMapper& m
)
{
    fixedValueFvPatchField<Type>::autoMap(m);
    referenceField_.autoMap(m);
    // Clear interpolator
    mapperPtr_.clear();
}


template<class Type>
void modifiedSEMFvPatchField<Type>::rmap
(
    const fvPatchField<Type>& ptf,
    const labelList& addr
)
{
    fixedValueFvPatchField<Type>::rmap(ptf, addr);

    const modifiedSEMFvPatchField<Type>& tiptf =
        refCast<const modifiedSEMFvPatchField<Type> >(ptf);

    referenceField_.rmap(tiptf.referenceField_, addr);
    // Clear interpolator
    mapperPtr_.clear();
}


template<class Type>
void modifiedSEMFvPatchField<Type>::updateCoeffs()
{
    if (this->updated())
    {
        return;
    }
	
	Field<Type>& patchField = *this;
	const polyMesh& pMesh = this->patch().boundaryMesh().mesh();

	//Info << "in update coef" << endl;

	bool verbos(false);
	scalar utau(sqrt(cf_*0.5*Uinf_[0]*Uinf_[0]));
	if (curTimeIndex_ == -1 )
	{
                
                //Experiments profile
		Info << "Reading Input Statistics" << endl;  
		scalarField samplePoints(List<scalar>(statisticsDict_.lookup("points")));
		symmTensorField sampleRplus(List<symmTensor>(statisticsDict_.lookup("R")));
		scalarField sampleUplus(List<scalar>(statisticsDict_.lookup("U")));
		tensorField Lund(sampleRplus.size(), pTraits<tensor>::zero);
		
		scalarField sampleU(sampleUplus*utau);
		symmTensorField sampleR(sampleRplus*utau*utau);

		//calculating maximum y
		maxy_= (samplePoints[samplePoints.size()-1])*delta_;


		Info << "Uinf = " << Uinf_ << endl;
		Info << "delta = " << delta_ << endl;
		Info << "Friction velocity = " << utau << endl;
		Info << "maximum y =" << maxy_ << endl;
		 
	}
       	//reading and updating eddy positions and calculating patch velocity if it needs an update
	if (curTimeIndex_ != this->db().time().timeIndex()      )
        {
		//symmTensor symTTest(1,2,3,4,5,6);
		//Info << "test XX = " << symTTest.component(symmTensor::XX) << endl;
                //Info << "test YY = " << symTTest.component(symmTensor::YY) << endl;
                //Info << "test ZZ = " << symTTest.component(symmTensor::ZZ) << endl;
                //Info << "test XY = " << symTTest.component(symmTensor::XY) << endl;
                //Info << "test XZ = " << symTTest.component(symmTensor::XZ) << endl;
                //Info << "test YZ = " << symTTest.component(symmTensor::YZ) << endl;
		//experiments profile
		scalarField samplePoints(List<scalar>(statisticsDict_.lookup("points")));
		symmTensorField sampleRplus(List<symmTensor>(statisticsDict_.lookup("R")));
		scalarField sampleUplus(List<scalar>(statisticsDict_.lookup("U")));
		tensorField Lund(sampleRplus.size(), pTraits<tensor>::zero);
		
		scalarField sampleU(sampleUplus*utau);
		symmTensorField sampleR(sampleRplus*utau*utau);

		//calculating maximum y
		maxy_= (samplePoints[samplePoints.size()-1])*delta_;



		Lund.replace(tensor::XX, sqrt(sampleR.component(symmTensor::XX)));
		Lund.replace(tensor::YX, sampleR.component(symmTensor::XY)/(Lund.component(tensor::XX)+ROOTVSMALL));
    		Lund.replace(tensor::ZX, sampleR.component(symmTensor::XZ)/(Lund.component(tensor::XX)+ROOTVSMALL));
		Lund.replace(tensor::YY, sqrt(sampleR.component(symmTensor::YY)-sqr(Lund.component(tensor::YX))));
    		Lund.replace(tensor::ZY, (sampleR.component(symmTensor::YZ) - Lund.component(tensor::YX)*Lund.component(tensor::ZX) )
					/(Lund.component(tensor::YY)+ROOTVSMALL));
		Lund.replace(tensor::ZZ, sqrt(sampleR.component(symmTensor::ZZ) - sqr(Lund.component(tensor::ZX))-sqr(Lund.component(tensor::ZY))));
		
		if(verbos)
		{
		
			forAll (samplePoints, pi)
			{
				Info << "Points =" << samplePoints[pi]<< endl;
				Info << "R =" << sampleR[pi] << endl;
				Info << "U =" << sampleU[pi] << endl;
				Info << "LUND =" << Lund[pi] << endl;

			}
		}
		//Info << "Lund =" << Lund << endl;
		int n_inter;
		n_inter = sampleR.size();
		//scalar deltastar=delta_/6.377; //look at the general slot hartnett excel file cell F52
		
			//defining Reynods stress tensor
		
		//symmTensorField R(100);
		//R[1]=(0.1 0 0 0.1 0 0.1);
                // getting bound box for patch
                boundBox bb(this->patch().patch().localPoints(), true);
                vector startPosition(bb.min()[0]-L_,bb.min()[1],bb.min()[2]);
                vector endPosition(bb.min()[0],min(maxy_,bb.max()[1]),bb.max()[2]);

                Info << "start = " << startPosition << endl;
                Info << "end = " << endPosition << endl;
                vectorField signedintensity(n_);
		
		vector  tempvector(0.5,0.5,0.5);
                vector  tempsign(1,1,1);
                //vector  unit(1,1,1);
		vector  tempi(fstu_,fstu_,fstu_);
		//genrating random positions for the first time
                bool isFirst(readBool(pointsDict_.lookup("isFirst")));
                if (isFirst)
                {

                        Info << "first time!!!" << endl;
                        //generating eddy positions
                        vector a(this->db().time().timeOutputValue(),1,1);      //dummy
                        forAll ( pp_,i )
                        {
                                pp_[i] = ranGen_.position(startPosition,endPosition);
                                
				//rndsign[i]=sign(tempsign);
                                sigma_[i] = ranGen_.position(sigmaMin_,sigmaMax_);
				intensity_[i] = tempi;
				//if (pp_[i][1]<=delta_) ////
				//{
                                //	intensity_[i] = (intensityMax_- tempi)*(1-pp_[i][1]/delta_)+tempi;
				//}
                                ranGen_.randomise(tempsign);
                                tempsign = tempsign - tempvector;
                                rndsign_[i][0]=sign(tempsign[0]);
                                rndsign_[i][1]=sign(tempsign[1]);
                                rndsign_[i][2]=sign(tempsign[2]);


                        }

                        pointsDict_.set("points",pp_);
                        pointsDict_.set("sigma",sigma_);
                        pointsDict_.set("intensity",intensity_);
                        pointsDict_.set("rndsign",rndsign_);
                        pointsDict_.set("time",this->db().time().timeOutputValue());
                        pointsDict_.set("isFirst",false);
                        pointsDict_.Foam::regIOobject::write();
			if(verbos)
			{
				Info << "positions = " << pp_ << endl;
				Info << "sigma = " << sigma_ << endl;
				Info << "intensity = " << intensity_ << endl;
				Info << "rndsign = " << rndsign_ << endl;
			}
                }
		else
                {
                        Info << "Conveting Eddies..." << endl;
                        //convecting eddies with mean velocity
                        vector a(this->db().time().timeOutputValue(),1,1);   //dummy
                        forAll ( pp_,i )
                        {
                                pp_[i]=pp_[i]+Uinf_*this->db().time().deltaTValue();
                                //checking if eddies convected outside of the box
                                if ( pp_[i][0] - sigma_[i][0] > bb.min()[0] )
                                {
                                        //generating new eddy
                                        //Info << "generating new eddy!!!!!!!!!!!!!!!!!1" << endl;
                                        pp_[i] = ranGen_.position(startPosition,endPosition);
					pp_[i][0]=startPosition[0];
                                        sigma_[i] = ranGen_.position(sigmaMin_,sigmaMax_);
					intensity_[i] = tempi;
				//if (pp_[i][1]<=delta_) ////
				//{
                                //	intensity_[i] = (intensityMax_- tempi)*(1-pp_[i][1]/delta_)+tempi;
				//}

                               	
					//intensity_[i] = (intensityMax_-tempi)*(1-pp_[i][1]/0.0043)+tempi;
                              		ranGen_.randomise(tempsign);
                                	tempsign = tempsign - tempvector;
                                	rndsign_[i][0]=sign(tempsign[0]);
                                	rndsign_[i][1]=sign(tempsign[1]);
                                	rndsign_[i][2]=sign(tempsign[2]);


		
                                }


                        }



                        pointsDict_.set("points",pp_);
                        pointsDict_.set("sigma",sigma_);
                        pointsDict_.set("intensity",intensity_);
                        pointsDict_.set("rndsign",rndsign_);

                        pointsDict_.set("time",this->db().time().timeOutputValue());
                        //Info << "points = " << pp_ << endl;
                }


                        //if(verbos)
                        //{

			//	Info << "out of else " << endl;
			//	Info << "positions = " << pp << endl;
			//	Info << "sigma = " << sigma << endl;
			//	Info << "intensity = " << intensity << endl;
			//	Info << "rndsign = " << rndsign << endl;
			//}

                scalar Vb((max(pp_ + sigma_)[0]-min(pp_ - sigma_)[0])*(endPosition[1]-bb.min()[1])*(bb.max()[2]-bb.min()[2]));
                //Info << "maximum x = " << max(pp + sigma)[0] << endl;         
                //Info << "minimum x = " << min(pp - sigma)[0] << endl; 
                //Info << "Vb      = " << Vb << endl;
                tensor a(1,0,0,0,1,0,0,0,1);

                //calculating patch velocity based on eddies current poistion
                //vectorField& patchField = *this;
                const vectorField& c = this->patch().Cf();
                vector dx(0,0,0);
                scalar f(0);
		Type test;
		vector unit(1,1,1);
		Type unitType(pTraits<Type>::one);
	        vector m(0,0,0);

	        scalar UCoeff(1);
		tensor LundCoeff(pTraits<tensor>::zero);
                forAll ( c,facei )
                {
                        patchField[facei] = pTraits<Type>::zero;//vector(0,0,0);
                        forAll(pp_,i)
                        {
                                dx = c[facei]-pp_[i];
                                dx[0]=dx[0]/sigma_[i][0];
                                dx[1]=dx[1]/sigma_[i][1];
                                dx[2]=dx[2]/sigma_[i][2];


                                f=1;
                                for(int j=0; j<3; j=j+1)
                                {
                                        if (mag(dx[j]) >= 1 )
                                        {
                                                f = 0;
                                        }
                                        else
                                        {
                                                //fout = fout * cos(pi/2*x(i))^2;
                                                //f = f * sqrt(scalar(5))*exp(-5*mag(dx[j]));  //1
                                                f = f * sqrt(scalar(1.5))*(1-mag(dx[j]));      //T21
                                                //f = f * sqrt(scalar(3.0)/sqrt(scalar(3.141592)))*exp(-9/2*sqr(dx[j])); //T22
                                                //fout = fout * 2*exp(-9*x(i)^2/2);
                                        }
                                }
				
				patchField[facei] = patchField[facei] +
                                                                        (
                                                                        simplify(rndsign_[i], pTraits<Type>::one)
									/sqrt(sigma_[i][0]*sigma_[i][1]*sigma_[i][2])
                                                                        * f
                                                                        * sqrt(Vb) / sqrt(n_)
                                                                        );

                        }
                        //interpolation on EXP profile
 			UCoeff=1;
			LundCoeff=pTraits<tensor>::zero;
 			for (int k=1; k<n_inter; k=k+1)
			{
				if (c[facei][1]<=samplePoints[k]*delta_)
				{
					UCoeff=(sampleU[k]-sampleU[k-1])/(samplePoints[k]-samplePoints[k-1])*(c[facei][1]-samplePoints[k-1])+sampleU[k-1];
					LundCoeff=(Lund[k]-Lund[k-1])/(samplePoints[k]-samplePoints[k-1])*(c[facei][1]-samplePoints[k-1])+Lund[k-1];
		break;
				}
			}	

			patchField[facei] = dotp(LundCoeff,patchField[facei]) + simplify(UCoeff*Uinf_,pTraits<Type>::one);


                        //Info << "y/delta =  " << c[facei][1]/delta_ << endl;
                        //Info << "facei    =  " << facei << endl;
                        //Info << "UCoef  =  " << UCoeff << endl;
                        //Info << "LundCoef  =  " << LundCoeff << endl;
			
                }
		Info << "Done!" << endl;
                curTimeIndex_ = this->db().time().timeIndex();
        }
        /*
        else
        {
                        Info << "Do not change positions,calculate velocity based on existing positions " << endl;
                        //reading positions             
                        pp              =vectorList(pointsDict.lookup("points"));
                        sigma           =vectorList(pointsDict.lookup("sigma"));
                        intensity       =vectorList(pointsDict.lookup("intensity"));
                        rndsign         =vectorList(pointsDict.lookup("rndsign"));
                        //Info << "in else " << endl;
                        //Info << "positions = " << pp << endl;
                        //Info << "sigma = " << sigma << endl;
                        //Info << "intensity = " << intensity << endl;
                        //Info << "rndsign = " << rndsign << endl;
        }
        */















fixedValueFvPatchField<Type>::updateCoeffs();
}



template<class Type>
void modifiedSEMFvPatchField<Type>::write(Ostream& os) const
{
    fvPatchField<Type>::write(os);
    os.writeKeyword("n") << n_ << token::END_STATEMENT << nl;
    os.writeKeyword("sigmaMin") << sigmaMin_ << token::END_STATEMENT << nl;
    os.writeKeyword("sigmaMax") << sigmaMax_ << token::END_STATEMENT << nl;
    os.writeKeyword("Uinf") << Uinf_ << token::END_STATEMENT << nl;
    os.writeKeyword("cf") << cf_ << token::END_STATEMENT << nl;
    os.writeKeyword("L") << L_ << token::END_STATEMENT << nl;
    os.writeKeyword("delta") << delta_ << token::END_STATEMENT << nl;
    os.writeKeyword("maxy") << maxy_ << token::END_STATEMENT << nl;
    os.writeKeyword("fstu") << fstu_ << token::END_STATEMENT << nl;
    referenceField_.writeEntry("referenceField", os);
    pointsDict_.Foam::regIOobject::write();
    this->writeEntry("value", os);
}


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

																																		// ************************************************************************* //
