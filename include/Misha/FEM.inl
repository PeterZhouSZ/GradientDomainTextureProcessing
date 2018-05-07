#include <math.h>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <atomic>

/////////
// FEM //
/////////

template< class Real >
inline Real FEM::Area( const SquareMatrix< Real , 2 >& tensor , const Point2D< Real > t[3] )
{
	return Dot( tensor , TangentVector< Real >( t[2] - t[0] ) , Rotate90( tensor , TangentVector< Real >( t[1] - t[0] ) ) ) / (Real)2;
}
template< class Real >
inline Real FEM::Area( const SquareMatrix< Real , 2 >& tensor , Point2D< Real > v1 , Point2D< Real > v2 , Point2D< Real > v3 )
{
	return Dot( tensor , TangentVector< Real >( v3 - v1 ) , Rotate90( tensor , TangentVector< Real >( v2 - v1 ) ) ) / (Real)2;
}
template< class Real >
inline Real FEM::Angle( const SquareMatrix< Real , 2 >& tensor , FEM::TangentVector< Real > v1 , FEM::TangentVector< Real > v2 )
{
	return (Real)acos( Dot( tensor , v1 , v2 ) / sqrt( SquareLength( tensor , v1 ) * SquareLength( tensor , v2 ) ) );
}
template< class Real >
inline Real FEM::Angle( const SquareMatrix< Real , 2 >& tensor , FEM::CotangentVector< Real > v1 , FEM::CotangentVector< Real > v2 )
{
	return (Real)acos( Dot( tensor , v1 , v2 ) / sqrt( SquareLength( tensor , v1 ) * SquareLength( tensor , v2 ) ) );
}
template< class Real >
inline SquareMatrix< Real , 2 > FEM::CoJ( const SquareMatrix< Real , 2 >& tensor )
{
	SquareMatrix< Real , 2 > J;
	J(0,1) = (Real)1 , J(1,0) = (Real)-1;
	return tensor * J / (Real)sqrt( tensor.determinant() );
}
template< class Real >
inline SquareMatrix< Real , 2 > FEM::J( const SquareMatrix< Real , 2 >& tensor )
{
	SquareMatrix< Real , 2 > J;
	J(0,1) = (Real)1 , J(1,0) = (Real)-1;
	return tensor.inverse() * J * (Real)sqrt( tensor.determinant() );
}
template< class Real > inline FEM::CotangentVector< Real > FEM::Rotate90( const SquareMatrix< Real , 2 >& tensor , FEM::CotangentVector< Real > v ){ return CoJ( tensor ) * v; }
template< class Real > inline FEM::  TangentVector< Real > FEM::Rotate90( const SquareMatrix< Real , 2 >& tensor , FEM::  TangentVector< Real > v ){ return   J( tensor ) * v; }

template< class Real > SquareMatrix< Real , 2 > FEM::MakeConformal( const SquareMatrix< Real , 2 >& sourceTensor , const SquareMatrix< Real , 2 >& targetTensor ){ return targetTensor * (Real)sqrt( sourceTensor.determinant() / targetTensor.determinant() ); }
template< class Real > SquareMatrix< Real , 2 > FEM::MakeAuthalic ( const SquareMatrix< Real , 2 >& sourceTensor , const SquareMatrix< Real , 2 >& targetTensor ){ return sourceTensor * (Real)sqrt( targetTensor.determinant() / sourceTensor.determinant() ); }
template< class Real >
SquareMatrix< Real , 2 > FEM::TensorRoot( const SquareMatrix< Real , 2 >& tensor )
{
	// Code borrowed from: https://en.wikipedia.org/wiki/Square_root_of_a_2_by_2_matrix
	SquareMatrix< Real , 2 > root = tensor;
	Real det = tensor.determinant();
	if( det<0 ) fprintf( stderr , "[ERROR] Negative determinant: %g\n" , det ) , exit( 0 );
	Real s = (Real)sqrt( det );
	Real disc = (Real)( tensor.trace() + 2. * s );
	if( disc<0 ) fprintf( stderr , "[ERROR] Negative discriminant: %g\n" , disc ) , exit( 0 );
	root(0,0) += s , root(1,1) += s;
	return root / (Real)sqrt( disc );
}
////////////////////////
// FEM::RightTriangle //
////////////////////////

template< class Real >
inline Point2D< Real > FEM::RightTriangle< Real >::Center( const SquareMatrix< Real , 2 >& tensor , int centerType )
{
	static const double SQRT_THREE_QUARTERS = sqrt( 3./4 );
	switch( centerType )
	{
	case CENTER_BARYCENTRIC:
		return Point2D< Real >( (Real)1./3 , (Real)1./3 );
	case CENTER_INCENTRIC:
		{
			Real lengths[] =
			{
				(Real)sqrt( Point2D< Real >::Dot( RightTriangle< Real >::EdgeDirections[0] , tensor * RightTriangle< Real >::EdgeDirections[0] ) ) ,
				(Real)sqrt( Point2D< Real >::Dot( RightTriangle< Real >::EdgeDirections[1] , tensor * RightTriangle< Real >::EdgeDirections[1] ) ) ,
				(Real)sqrt( Point2D< Real >::Dot( RightTriangle< Real >::EdgeDirections[2] , tensor * RightTriangle< Real >::EdgeDirections[2] ) )
			};
			Real lSum = lengths[0] + lengths[1] + lengths[2];
			return Point2D< Real >( lengths[1] / lSum , lengths[2] / lSum );
		}
	case CENTER_CIRCUMCENTRIC:
		{
			Real maxDet = 0;
			Point2D< Real > c;
			for( int j=0 ; j<3 ; j++ )
			{
				Point2D< Real > c1 = RightTriangle< Real >::EdgeMidpoints[(j+1)%3] , c2 = RightTriangle< Real >::EdgeMidpoints[(j+2)%3];
				Point2D< Real > v1 = Rotate90( tensor , RightTriangle< Real >::EdgeDirections[(j+1)%3] ) , v2 = Rotate90( tensor , RightTriangle< Real >::EdgeDirections[(j+2)%3] );
				// Solve for s and t such that:
				//		c1 + s * v1 = c2 + t * v2
				// =>	c1 - c2 = - s * v1 + t * v2
				// =>	c1 - c2 = ( -v1 | v2 ) * ( s , t )^t
				SquareMatrix< Real , 2 > M;
				M( 0 , 0 ) = -v1[0] , M( 0 , 1 ) = -v1[1];
				M( 1 , 0 ) =  v2[0] , M( 1 , 1 ) =  v2[1];
				Real det = (Real)fabs( M.determinant() );
				if( det>maxDet )
				{
					Point2D< Real > x = M.inverse() * ( c1 - c2 );
					c = ( c1 + v1 * x[0] + c2 + v2 * x[1] ) / 2;
					maxDet = det;
				}
			}
			return c;
		}
	case CENTER_ISOGONIC:
		{
			Point2D< Real > eVertices[3];
			// Solve for the positive value of t such that:
			//		|| Corners[j+1] - ( midPoint + t * perpDir ) ||^2 = || Corners[j+1] - Corners[j+2] ||^2
			// <=>	t^2 || perpDir ||^2 = || Corners[j+1] - Corners[j+2] ||^2 - || Corners[j+1] - midPoint ||^2
			// <=>	t^2 || Corners[j+1] - Corners[j+2] ||^2 = || Corners[j+1] - Corners[j+2] ||^2 - || ( Corners[j+1] - Corners[j+2] )/2 ||^2
			// <=>	t^2 || Corners[j+1] - Corners[j+2] ||^2 = 3/4 || Corners[j+1] - Corners[j+2] ||^2
			// <=>	t = sqrt( 3/4 )
			for( int j=0 ; j<3 ; j++ ) eVertices[j] = RightTriangle< Real >::EdgeMidpoints[j] - Rotate90( tensor ,  RightTriangle< Real >::EdgeDirections[j] ) * (Real)SQRT_THREE_QUARTERS;
			Real maxDet = 0;
			Point2D< Real > c;
			for( int j=0 ; j<3 ; j++ )
			{
				Point2D< Real > c1 = eVertices[(j+1)%3] , c2 = eVertices[(j+2)%3];
				Point2D< Real > v1 = RightTriangle< Real >::Corners[(j+1)%3] - c1 , v2 = RightTriangle< Real >::Corners[(j+2)%3] - c2;

				// Solve for s and t such that:
				//		c1 + s * v1 = c2 + t * v2
				// =>	c1 - c2 = - s * v1 + t * v2
				// =>	c1 - c2 = ( -v1 | v2 ) * ( s , t )^t
				SquareMatrix< Real , 2 > M;
				M( 0 , 0 ) = -v1[0] , M( 0 , 1 ) = -v1[1];
				M( 1 , 0 ) =  v2[0] , M( 1 , 1 ) =  v2[1];
				Real det = (Real)fabs( M.determinant() );
				if( det>maxDet )
				{
					Point2D< Real > x = M.inverse() * ( c1 - c2 );
					c = ( c1 + v1 * x[0] + c2 + v2 * x[1] ) / 2;
					maxDet = det;
				}
			}
			return c;
		}
	default:
		fprintf( stderr , "[ERROR] Unrecognized dual type: %d\n" , centerType ) , exit( 0 );
	}
}
template< class Real >
inline Point3D< Real > FEM::RightTriangle< Real >::SubTriangleAreas( const SquareMatrix< Real , 2 >& tensor , Point2D< Real > center )
{
	Point2D< Real > triangle[3];
	triangle[2] = center;
	Point3D< Real > areas;
	for( int i=0 ; i<3 ; i++ )
	{
		triangle[0] = RightTriangle< Real >::Corners[(i+1)%3] , triangle[1] = RightTriangle< Real >::Corners[(i+2)%3];
		areas[i] = Area( tensor , triangle );
	}
	return areas;
}
template< class Real >
inline Point3D< Real > FEM::RightTriangle< Real >::CenterAreas( const SquareMatrix< Real , 2 >& tensor , int centerType ){ return SubTriangleAreas( tensor , Center( tensor , centerType ) ); }

template< class Real >
template< unsigned int BasisType , class V >
inline V FEM::RightTriangle< Real >::EvaluateScalarField( ConstPointer( V ) c , const Point2D< Real >& p )
{
	switch( BasisType )
	{
		case BASIS_0_WHITNEY: return c[0] * (Real)( 1. - p[0] - p[1] ) + c[1] * p[0] + c[2] * p[1];
		default: fprintf( stderr , "[ERROR] FEM::RightTriangle::EvaluateScalarField: unrecognized basis type: %d\n" , BasisType ) , exit( 0 );
	}
}
template< class Real >
template< unsigned int BasisType , class V >
FEM::CotangentVector< V > FEM::RightTriangle< Real >::EvaluateScalarFieldGradient( ConstPointer( V ) c , const Point2D< Real >& p )
{
	switch( BasisType )
	{
		case BASIS_0_WHITNEY: return CotangentVector< V >( c[0]*CornerGradients[0][0] + c[1]*CornerGradients[1][0] + c[2]*CornerGradients[2][0] , c[0]*CornerGradients[0][1] + c[1]*CornerGradients[1][1] + c[2]*CornerGradients[2][1] );
		default: fprintf( stderr , "[ERROR] FEM::RightTriangle::EvaluateScalarField: unrecognized basis type: %d\n" , BasisType ) , exit( 0 );
	}
}
template< class Real >
template< unsigned int BasisType , class V >
inline FEM::CotangentVector< V > FEM::RightTriangle< Real >::EvaluateCovectorField( const SquareMatrix< Real , 2 >& tensor , ConstPointer( V ) c , const Point2D< Real >& p )
{
	switch( BasisType )
	{
		case BASIS_1_CONFORMING:
		{
			SquareMatrix< Real , 2 > J = CoJ( tensor );
			CotangentVector< Real > g[] = { CornerGradients[0] , J *CornerGradients[0] , CornerGradients[1] , J * CornerGradients[1] , CornerGradients[2] , J * CornerGradients[2] };
			return CotangentVector< V >( c[0]*g[0][0] + c[1]*g[1][0] + c[2]*g[2][0] + c[3]*g[3][0] + c[4]*g[4][0] + c[5]*g[5][0] , c[0]*g[0][1] + c[1]*g[1][1] + c[2]*g[2][1] + c[3]*g[3][1] + c[4]*g[4][1] + c[5]*g[5][1] );
		}
		case BASIS_1_WHITNEY:
		{
			Point3D< Real > v( (Real)( 1. - p[0] - p[1] ) , p[0] , p[1] );
			V n[] = { c[1]*v[2] - c[2]*v[1] , c[2]*v[0] - c[0]*v[2] , c[0]*v[1] - c[1]*v[0] };
			return CotangentVector< V >( n[0]*CornerGradients[0][0] + n[1]*CornerGradients[1][0] + n[2]*CornerGradients[2][0] , n[0]*CornerGradients[0][0] + n[1]*CornerGradients[1][1] + n[2]*CornerGradients[2][1] );
		}
		case BASIS_1_TRIANGLE_CONSTANT: return CotangentVector< V >( c[0] , c[1] );
		default: fprintf( stderr , "[ERROR] FEM::RightTriangle::EvaluateCovectorField: unrecognized basis type: %d\n" , BasisType ) , exit( 0 );
	}
}
template< class Real >
template< unsigned int BasisType , class V >
inline V FEM::RightTriangle< Real >::EvaluateDensityField( const SquareMatrix< Real , 2 >& tensor , ConstPointer( V ) c , const Point2D< Real >& p )
{
	switch( BasisType )
	{
		case BASIS_2_WHITNEY: return c[0] / Area( tensor );
		default: fprintf( stderr , "[ERROR] FEM::RightTriangle::EvaluateVectorField: unrecognized basis type: %d\n" , BasisType ) , exit( 0 );
	}
}
template< class Real >
template< unsigned int InBasisType , unsigned int OutBasisType >
Matrix< unsigned char , FEM::BasisInfo< InBasisType >::Coefficients , FEM::BasisInfo< OutBasisType >::Coefficients > FEM::RightTriangle< Real >::GetDMask( bool& redundant )
{
	auto Fail = [&] ( void ){ fprintf( stderr , "[ERROR] FEM::RightTriangle::GetDMask: %s -> %s\n" , BasisNames[ InBasisType ] , BasisNames[ OutBasisType ] ) , exit( 0 ); };

	TestBasisType(  InBasisType , "FEM::RightTriangle::GetDMask" , false );
	TestBasisType( OutBasisType , "FEM::RightTriangle::GetDMask" , false );

	Matrix< unsigned char , FEM::BasisInfo< InBasisType >::Coefficients , FEM::BasisInfo< OutBasisType >::Coefficients > D;
	D *= 0;

	if( InBasisType==BASIS_0_WHITNEY )
	{
		if     ( OutBasisType==BASIS_1_WHITNEY           ) for( int e=0 ; e<3 ; e++ ) D( (e+1)%3 , e ) = D( (e+2)%3 , e ) = 1;
		else if( OutBasisType==BASIS_1_CONFORMING        ) for( int v=0 ; v<3 ; v++ ) D( v , v*2 ) = 1;
		else if( OutBasisType==BASIS_1_TRIANGLE_CONSTANT ) for( int i=0 ; i<2 ; i++ ) D( 0 , i ) = D( 1+i , i ) = 1;
		else Fail();
	}
	else if( OutBasisType==BASIS_2_VERTEX_CONSTANT )
	{
		if     ( InBasisType==BASIS_1_CONFORMING                                        ) for( int v=0 ; v<3 ; v++ ) for( int i=0 ; i<3 ; i++ ) D( i*2+1 , v ) = 1;
		else if( InBasisType==BASIS_1_WHITNEY || InBasisType==BASIS_1_TRIANGLE_CONSTANT ) for( int v=0 ; v<3 ; v++ ) for( int i=0 ; i<BasisInfo< InBasisType >::Coefficients ; i++ ) D(i,v) = 1;
		else Fail();
	}
	else if( InBasisType==BASIS_1_WHITNEY )
	{
		if( OutBasisType==BASIS_2_WHITNEY ) D(0,0) = D(1,0) = D(2,0) = 1;
		else Fail();
	}
	else Fail();
	redundant = ( OutBasisType == BASIS_1_WHITNEY || OutBasisType==BASIS_1_CONFORMING );
	return D;
}
template< class Real >
template< unsigned int InBasisType , unsigned int OutBasisType >
Matrix< Real , FEM::BasisInfo< InBasisType >::Coefficients , FEM::BasisInfo< OutBasisType >::Coefficients > FEM::RightTriangle< Real >::GetDMatrix( const SquareMatrix< Real , 2 >& tensor )
{
	auto Fail = [&] ( void ){ fprintf( stderr , "[ERROR] FEM::RightTriangle::GetDMatrix: %s -> %s\n" , BasisNames[ InBasisType ] , BasisNames[ OutBasisType ] ) , exit( 0 ); };

	TestBasisType(  InBasisType , "FEM::RightTriangle::GetDMatrix" , false );
	TestBasisType( OutBasisType , "FEM::RightTriangle::GetDMatrix" , false );

	Matrix< Real , FEM::BasisInfo< InBasisType >::Coefficients , FEM::BasisInfo< OutBasisType >::Coefficients > D;

	if( InBasisType==BASIS_0_WHITNEY )
	{
		if     ( OutBasisType==BASIS_1_WHITNEY           ) for( int e=0 ; e<3 ; e++ ) D( (e+1)%3 , e ) = -1 , D( (e+2)%3 , e ) = 1;
		else if( OutBasisType==BASIS_1_CONFORMING        ) for( int v=0 ; v<3 ; v++ ) D( v , v*2 ) = 1;
		else if( OutBasisType==BASIS_1_TRIANGLE_CONSTANT ) for( int i=0 ; i<2 ; i++ ) D( 0 , i ) = -1 , D( 1+i , i ) = 1;
		else Fail();
	}
	else if( OutBasisType==BASIS_2_VERTEX_CONSTANT )
	{
		if( InBasisType==BASIS_1_WHITNEY || InBasisType==BASIS_1_CONFORMING || InBasisType==BASIS_1_TRIANGLE_CONSTANT )
		{
			Point2D< Real > center = Center( tensor , CENTER_CIRCUMCENTRIC );
			for( int v=0 ; v<3 ; v++ )
			{
				Point< Real , BasisInfo< InBasisType >::Coefficients > d = IntegrationDual< InBasisType >( tensor , EdgeMidpoints[(v+2)%3] , center ) + IntegrationDual< InBasisType >( tensor , center , EdgeMidpoints[(v+1)%3] );
				for( int i=0 ; i<BasisInfo< InBasisType >::Coefficients ; i++ ) D(i,v) = d[i];
			}

		}
		else Fail();
	}
	else if( InBasisType==BASIS_1_WHITNEY )
	{
		if( OutBasisType==BASIS_2_WHITNEY ) D(0,0) = D(1,0) = D(2,0) = 1;
		else Fail();
	}
	else Fail();
	return D;
}
template< class Real >
template< unsigned int BasisType >
SquareMatrix< Real , FEM::BasisInfo< BasisType >::Coefficients > FEM::RightTriangle< Real >::GetMassMatrix( const SquareMatrix< Real , 2 >& tensor )
{
	SquareMatrix< Real , BasisInfo< BasisType >::Coefficients > mass;

	switch( BasisType )
	{
		case BASIS_0_WHITNEY:
		{
			{
				mass(0,0) = mass(1,1) = mass(2,2) = (Real)1./6;
				mass(1,0) = mass(0,1) = mass(1,2) = mass(2,1) = mass(0,2) = mass(2,0) = (Real)1./12;		
				mass *= Area( tensor , Corners );
			}
			break;
		}
		case BASIS_1_CONFORMING:
		{
			{
				SquareMatrix< Real , 2 > iTensor = tensor.inverse() * Area( tensor , Corners );
				for( int i=0 ; i<3 ; i++ ) for( int j=0 ; j<3 ; j++ ) mass( 2*i , 2*j ) = mass( 2*i+1 , 2*j+1 ) = Point2D< Real >::Dot( CornerGradients[i] , iTensor * CornerGradients[j] );
			}
			break;
		}
		case BASIS_1_WHITNEY:
		{
			{
				SquareMatrix< Real , 3 > M = GetMassMatrix< BASIS_0_WHITNEY >( tensor ) , S;
				SquareMatrix< Real , 2 > iTensor = tensor.inverse();
				for( int i=0 ; i<3 ; i++ ) for( int j=0 ; j<3 ; j++ ) S(i,j) = Point2D< Real >::Dot( CornerGradients[i] , iTensor * CornerGradients[j] );
		
				const int I[][2] = { {1,2} , {2,0} , {0,1} };
				for( int i=0 ; i<3;  i++ ) for( int j=0 ; j<3 ; j++ )
					mass(i,j) = M( I[i][0] , I[j][0] ) * S( I[i][1] , I[j][1] ) + M( I[i][1] , I[j][1] ) * S( I[i][0] , I[j][0] ) - M( I[i][0] , I[j][1] ) * S( I[i][1] , I[j][0] ) - M( I[i][1] , I[j][0] ) * S( I[i][0] , I[j][1] );
			}
			break;
		}
		case BASIS_1_TRIANGLE_CONSTANT:
		{
			SquareMatrix< Real , 2 > iTensor = tensor.inverse();
			for( int i=0 ; i<2 ; i++ ) for( int j=0 ; j<2 ; j++ ) mass(i,j) = iTensor(i,j);
			mass *= Area( tensor , Corners );
			break;
		}
		case BASIS_2_WHITNEY:
			{	
				mass(0,0) = (Real)( 1./Area( tensor , Corners ) );
			}
			break;
		default: TestBasisType( BasisType , "FEM::RightTriangle::GetMassMatrix" , true );
	}

	return mass;
}
template< class Real >
template< unsigned int BasisType >
SquareMatrix< unsigned char , FEM::BasisInfo< BasisType >::Coefficients > FEM::RightTriangle< Real >::GetMassMask( bool useTensor )
{
	SquareMatrix< unsigned char , BasisInfo< BasisType >::Coefficients > M;
	M *= 0;

	if( useTensor )
		switch( BasisType )
		{
			case BASIS_0_WHITNEY:           for( int i=0 ; i<3 ; i++ ) for( int j=0 ; j<3 ; j++ ) M(i,j) = 1 ; break;
			case BASIS_1_CONFORMING:        for( int i=0 ; i<6 ; i++ ) for( int j=0 ; j<6 ; j++ ) M(i,j) = 1 ; break;
			case BASIS_1_WHITNEY:           for( int i=0 ; i<3 ; i++ ) for( int j=0 ; j<3 ; j++ ) M(i,j) = 1 ; break;
			case BASIS_1_TRIANGLE_CONSTANT: for( int i=0 ; i<2 ; i++ ) for( int j=0 ; j<2 ; j++ ) M(i,j) = 1 ; break;
			case BASIS_2_WHITNEY:                                                                 M(0,0) = 1 ; break;
			default: TestBasisType( BasisType , "FEM::RightTriangle::GetMassMask" , true );
		}
	else
		switch( BasisType )
		{
			case BASIS_0_WHITNEY:           for( int i=0 ; i<3 ; i++ ) for( int j=0 ; j<3 ; j++ ) M(i,j) = 1 ; break;
			case BASIS_1_CONFORMING:        for( int i=0 ; i<3 ; i++ ) for( int j=0 ; j<3 ; j++ ) M(2*i,2*j) = M(2*i+1,2*j+1) = 1 ; break;
			case BASIS_1_WHITNEY:           for( int i=0 ; i<3 ; i++ ) for( int j=0 ; j<3 ; j++ ) M(i,j) = 1 ; break;
			case BASIS_1_TRIANGLE_CONSTANT: for( int i=0 ; i<2 ; i++ ) for( int j=0 ; j<2 ; j++ ) M(i,j) = 1 ; break;
			case BASIS_2_WHITNEY:                                                                 M(0,0) = 1 ; break;
			default: TestBasisType( BasisType , "FEM::RightTriangle::GetMassMask" , true );
		}
	return M;
}
template< class Real >
template< unsigned int BasisType >
Point< Real , FEM::BasisInfo< BasisType >::Coefficients > FEM::RightTriangle< Real >::GetDiagonalMassMatrix( const SquareMatrix< Real , 2 >& tensor )
{
	Point< Real , BasisInfo< BasisType >::Coefficients > mass;

	switch( BasisType )
	{
		case BASIS_0_WHITNEY:
			mass[0] = mass[1] = mass[2] = Area( tensor , Corners ) / (Real)3.;
			break;
		case BASIS_1_CONFORMING:
		{
			Point3D< Real > areas = CenterAreas( tensor , CENTER_CIRCUMCENTRIC )*2;
			mass[0] = mass[1] = areas[0] / SquareLength( tensor , EdgeDirections[0] );
			mass[2] = mass[3] = areas[1] / SquareLength( tensor , EdgeDirections[1] );
			mass[4] = mass[5] = areas[2] / SquareLength( tensor , EdgeDirections[2] );
			break;
		}
		case BASIS_1_WHITNEY:
		{
			Point3D< Real > areas = CenterAreas( tensor , CENTER_CIRCUMCENTRIC )*2;
			mass[0] = areas[0] / SquareLength( tensor , EdgeDirections[0] );
			mass[1] = areas[1] / SquareLength( tensor , EdgeDirections[1] );
			mass[2] = areas[2] / SquareLength( tensor , EdgeDirections[2] );
			break;
		}
		case BASIS_2_WHITNEY:
			mass[0] = (Real)( 1./Area( tensor , RightTriangle< Real >::Corners ) );
			break;
		default: TestBasisType( BasisType , "FEM::RightTriangle::GetDiagonalMassMatrix" , true );
	}

	return mass;
}
template< class Real >
template< unsigned int BasisType >
SquareMatrix< Real , FEM::BasisInfo< BasisType >::Coefficients > FEM::RightTriangle< Real >::GetMassMatrix( const SquareMatrix< Real , 2 >& tensor , const SquareMatrix< Real , 2 >& newTensor )
{
	SquareMatrix< Real , BasisInfo< BasisType >::Coefficients > mass;
	SquareMatrix< Real , 2 > tensor_i = tensor.inverse() , cotensor = tensor_i.transpose() * newTensor * tensor_i;

	switch( BasisType )
	{
		case BASIS_0_WHITNEY:
		{
			mass(0,0) = mass(1,1) = mass(2,2) = (Real)1./6;
			mass(1,0) = mass(0,1) = mass(1,2) = mass(2,1) = mass(0,2) = mass(2,0) = (Real)1./12;		
			mass *= Area( tensor , Corners );
			break;
		}
		case BASIS_1_CONFORMING:
		{
			CotangentVector< Real > grads[] = { CornerGradients[0] , Rotate90( tensor , CornerGradients[0] ) , CornerGradients[1] , Rotate90( tensor , CornerGradients[1] ) , CornerGradients[2] , Rotate90( tensor , CornerGradients[2] ) };
			for( int i=0 ; i<6 ; i++ ) for( int j=0 ; j<6 ; j++ ) mass(i,j) = Point2D< Real >::Dot( grads[i]  , cotensor * grads[j] );
			mass *= Area( tensor , Corners );
			break;
		}
		case BASIS_1_TRIANGLE_CONSTANT:
		{
			for( int i=0 ; i<2 ; i++ ) for( int j=0 ; j<2 ; j++ ) mass(i,j) = cotensor(i,j);
			mass *= Area( tensor , Corners );
			break;
		}
		case BASIS_1_WHITNEY:
		{
			SquareMatrix< Real , 3 > M = GetMassMatrix< BASIS_0_WHITNEY >( tensor ) , S;
			for( int i=0 ; i<3 ; i++ ) for( int j=0 ; j<3 ; j++ ) S(i,j) = Point2D< Real >::Dot( CornerGradients[i] , cotensor * CornerGradients[j] );
		
			const int I[][2] = { {1,2} , {2,0} , {0,1} };
			for( int i=0 ; i<3;  i++ ) for( int j=0 ; j<3 ; j++ )
				mass(i,j) = M( I[i][0] , I[j][0] ) * S( I[i][1] , I[j][1] ) + M( I[i][1] , I[j][1] ) * S( I[i][0] , I[j][0] ) - M( I[i][0] , I[j][1] ) * S( I[i][1] , I[j][0] ) - M( I[i][1] , I[j][0] ) * S( I[i][0] , I[j][1] );
			break;
		}
		case BASIS_2_WHITNEY:
			mass(0,0) = (Real)( cotensor.determinant() / Area( tensor , Corners ) );
			break;
		default: TestBasisType( BasisType , "FEM::RightTriangle::GetMassMatrix" , true );
	}

	return mass;
}
template< class Real >
template< unsigned int BasisType >
Real FEM::RightTriangle< Real >::Integrate( const SquareMatrix< Real , 2 >& tensor , ConstPointer( Real ) linear )
{
	Real integral = 0;
	switch( BasisType )
	{
		case BASIS_0_WHITNEY:
		{
			SquareMatrix< Real , 3 > mass = GetMassMatrix< BASIS_0_WHITNEY >( tensor );
			for( int i=0 ; i<3 ; i++ ) integral += linear[i] * ( mass(i,0) + mass(i,1) + mass(i,2) );
			break;
		}
		default: TestBasisType( BasisType , "FEM::RightTriangle::Integrate" , true );
	}
	return integral;
}
template< class Real >
template< unsigned int BasisType >
Point< Real , FEM::BasisInfo< BasisType >::Coefficients > FEM::RightTriangle< Real >::IntegrationDual( const SquareMatrix< Real , 2 >& tensor , ConstPointer( Real ) linear )
{
	Point< Real , BasisInfo< BasisType >::Coefficients > integralDual;
	switch( BasisType )
	{
		case BASIS_0_WHITNEY:
		{
			SquareMatrix< Real , 3 > mass = GetMassMatrix< BASIS_0_WHITNEY >( tensor );
			for( int i=0 ; i<3 ; i++ ) for( int j=0 ; j<3 ; j++ ) integralDual[i] += linear[j] * mass(i,j);
			break;
		}
		default: TestBasisType( BasisType , "FEM::RightTriangle::IntegrationDual" , true );
	}
	return integralDual;
}
template< class Real >
template< unsigned int BasisType >
Point< Real , FEM::BasisInfo< BasisType >::Coefficients > FEM::RightTriangle< Real >::IntegrationDual( const SquareMatrix< Real , 2 >& tensor , ConstPointer( CotangentVector< Real > ) linear )
{
	Point< Real , BasisInfo< BasisType >::Coefficients > integralD;
	switch( BasisType )
	{
		case BASIS_1_TRIANGLE_CONSTANT:
		{
			SquareMatrix< Real , 2 > tensor_i = tensor.inverse();
			Point2D< Real > dualBasis[] = { tensor_i * Point2D< Real >( 1 , 0 ) , tensor_i * Point2D< Real >( 0 , 1 ) };
			Real _linear[3];
			for( int i=0 ; i<2 ; i++ )
			{
				for( int j=0 ; j<3 ; j++ ) _linear[j] = Point2D< Real >::Dot( dualBasis[i] , linear[j] );
				integralD[i] = Integrate< BASIS_0_WHITNEY >( tensor , ( ConstPointer( Real ) )GetPointer( _linear , 3 ) );
			}
			break;
		}
		case BASIS_1_CONFORMING:
		{
			SquareMatrix< Real , 2 > tensor_i = tensor.inverse();
			Point2D< Real > dualBasis[] =
			{
				tensor_i * CornerGradients[0] , tensor_i * Rotate90( tensor , CornerGradients[0] ) ,
				tensor_i * CornerGradients[1] , tensor_i * Rotate90( tensor , CornerGradients[1] ) ,
				tensor_i * CornerGradients[2] , tensor_i * Rotate90( tensor , CornerGradients[2] )
			};
			Real _linear[3];
			for( int i=0 ; i<6 ; i++ )
			{
				for( int j=0 ; j<3 ; j++ ) _linear[j] = Point2D< Real >::Dot( dualBasis[i] , linear[j] );
				integralD[i] = Integrate< BASIS_0_WHITNEY >( tensor , ( ConstPointer( Real ) )GetPointer( _linear , 3 ) );
			}
			break;
		}
		case BASIS_1_WHITNEY:
		{
			// \phi_{ij}(p) = \phi_i(p) \nabla \phi_j - \phi_j(p) \nabla \phi_i
			// phi[i](p) = hat[i+1](p) * CornerGradients[i+2] - hat[i+2](p) * CornerGradients[i+1]
			// < phi[i](p) , L(p) > = \sum_j < phi[i](p) , linear[j] * hat[j](p) >
			//                      = \sum_j < hat[i+1](p) * CornerGradients[i+2] - hat[i+2](p) * CornerGradients[i+1] , linear[j] * hat[j](p) >
			//                      = \sum_j < CornerGradients[i+2] , linear[j] > * Integral{ hat[i+1](p) * hat[j](p) } - < CornerGradients[i+1] , linear[j] >  * Integral{ hat[i+2](p) * hat[j](p) }
			SquareMatrix< Real , 3 > mass = GetMassMatrix< BASIS_0_WHITNEY >( tensor );
			SquareMatrix< Real , 2 > tensor_i = tensor.inverse();
			for( int j=0 ; j<3 ; j++ )
			{
				CotangentVector< Real > linearD = tensor_i * linear[j];
				for( int i=0 ; i<3 ; i++ ) 
				{
					integralD[i] += mass( (i+1)%3 , j ) * Point2D< Real >::Dot( CornerGradients[(i+2)%3] , linearD );
					integralD[i] -= mass( (i+2)%3 , j ) * Point2D< Real >::Dot( CornerGradients[(i+1)%3] , linearD );
				}
			}
			break;
		}
		default: TestBasisType( BasisType , "FEM::RightTriangle::IntegrationDual" , true );
	}
	return integralD;
}
template< class Real >
template< unsigned int BasisType >
Point< Real , FEM::BasisInfo< BasisType >::Coefficients > FEM::RightTriangle< Real >::IntegrationDual( const SquareMatrix< Real , 2 >& tensor , Point2D< Real > p , Point2D< Real > q )
{
	Point< Real , BasisInfo< BasisType >::Coefficients > integralDual , coefficients;
	switch( BasisType )
	{
		case BASIS_1_WHITNEY:
		case BASIS_1_CONFORMING:
		case BASIS_1_TRIANGLE_CONSTANT:
		{
			TangentVector< Real >dir = q - p;
			for( int i=0 ; i<BasisInfo< BasisType >::Coefficients ; i++ )
			{
				coefficients[i] = 1;
				CotangentVector< Real > _p = EvaluateCovectorField< BasisType >( tensor , ( ConstPointer( Real ) )GetPointer( &coefficients[0] , BasisInfo< BasisType >::Coefficients ) , p );
				CotangentVector< Real > _q = EvaluateCovectorField< BasisType >( tensor , ( ConstPointer( Real ) )GetPointer( &coefficients[0] , BasisInfo< BasisType >::Coefficients ) , q );
				integralDual[i] = ( Point2D< Real >::Dot( dir , _p ) + Point2D< Real >::Dot( dir , _q ) ) / 2;
				coefficients[i] = 0;
			}
			break;
		}
		default: TestBasisType( BasisType , "FEM::RightTriangle::IntegrationDual" , true );
	}
	return integralDual;
}

////////////////////////////////////////////////////
// FEM::RiemannianMesh::TangentVectorFieldWrapper //
////////////////////////////////////////////////////
template< class Real , unsigned int BasisType >
FEM::TangentVectorFieldWrapper< Real , BasisType >::TangentVectorFieldWrapper( const RiemannianMesh< Real >* mesh , ConstPointer( Real ) coefficients , bool precomputeInverses ) : _mesh(mesh) , _coefficients(coefficients)
{
	if( precomputeInverses )
	{
		_gInverse = AllocPointer< SquareMatrix< Real , 2 > >( _mesh->tCount() );
#pragma omp parallel for
		for( int t=0 ; t<_mesh->tCount() ; t++ ) _gInverse[t] = _mesh->g(t).inverse();
	}
	else _gInverse = NullPointer< SquareMatrix< Real , 2 > >();
}
template< class Real , unsigned int BasisType >
FEM::TangentVectorFieldWrapper< Real , BasisType >::~TangentVectorFieldWrapper( void ){ FreePointer( _gInverse ); }
template< class Real , unsigned int BasisType >
FEM::TangentVector< Real > FEM::TangentVectorFieldWrapper< Real , BasisType >::operator() ( const FEM::SamplePoint< Real >& p ) const
{
	bool isAligned;
	Real c[BasisInfo< BasisType >::Coefficients];
	for( int i=0 ; i<BasisInfo< BasisType >::Coefficients ; i++ )
	{
		unsigned int idx =_mesh->index< BasisType >( p.tIdx , i , isAligned );
		c[i] = isAligned ? _coefficients[ idx ] : -_coefficients[ idx ] ;
	}
	if( _gInverse ) return _gInverse[ p.tIdx ]          * RightTriangle< Real >::EvaluateCovectorField< BasisType >( _mesh->g(p.tIdx) , ( ConstPointer( Real ) )GetPointer( &c[0] , BasisInfo< BasisType >::Coefficients ) , p.p );
	else            return _mesh->g( p.tIdx ).inverse() * RightTriangle< Real >::EvaluateCovectorField< BasisType >( _mesh->g(p.tIdx) , ( ConstPointer( Real ) )GetPointer( &c[0] , BasisInfo< BasisType >::Coefficients ) , p.p );
}

/////////////////////////
// FEM::RiemannianMesh //
/////////////////////////
template< class Real >
FEM::RiemannianMesh< Real >::RiemannianMesh( Pointer( TriangleIndex ) t , size_t tC ) : _triangles(t) , _tCount(tC) , _edgeMap( t , tC )
{
	_g = AllocPointer< SquareMatrix< Real , 2 > >( _tCount );
	for( int t=0 ; t<_tCount ; t++ ) _g[t] = SquareMatrix< Real , 2 >::Identity();
	_vCount = 0;
	for( unsigned int i=0 ; i<_tCount ; i++ ) for( int j=0 ; j<3 ; j++ ) _vCount = std::max< size_t >( _vCount , _triangles[i][j]+1 );
}
template< class Real >
FEM::RiemannianMesh< Real >::~RiemannianMesh( void ){ FreePointer( _g ); }
template< class Real >
template< unsigned int BasisType >
size_t FEM::RiemannianMesh< Real >::dimension( void ) const
{
	size_t dim;
	switch( BasisInfo< BasisType >::ElementType )
	{
		case ELEMENT_VERTEX:   dim = vCount() * BasisInfo< BasisType >::CoefficientsPerElement ; break;
		case ELEMENT_EDGE:     dim = eCount() * BasisInfo< BasisType >::CoefficientsPerElement ; break;
		case ELEMENT_TRIANGLE: dim = tCount() * BasisInfo< BasisType >::CoefficientsPerElement ; break;
		default:
			TestBasisType( BasisType , "FEM::RiemannianMesh::dimension" , false );
			TestElementType( BasisInfo< BasisType >::ElementType , "FEM::RiemannianMesh::dimension" , true );
	}
	return dim;
}
template< class Real >
template< unsigned int BasisType >
unsigned int FEM::RiemannianMesh< Real >::index( unsigned int t , unsigned int idx ) const
{
	bool isAligned;
	return index< BasisType >( t , idx , isAligned );
}
template< class Real >
template< unsigned int BasisType >
unsigned int FEM::RiemannianMesh< Real >::index( unsigned int t , unsigned int idx , bool& isAligned ) const
{
	isAligned = true;
	int i , e = idx / BasisInfo< BasisType >::CoefficientsPerElement , c = idx % BasisInfo< BasisType >::CoefficientsPerElement;
	switch( BasisInfo< BasisType >::ElementType )
	{
		case ELEMENT_VERTEX:   i = _triangles[t][e]                   ; break;
		case ELEMENT_EDGE:     i = _edgeMap.edge( t*3+e , isAligned ) ; break;
		case ELEMENT_TRIANGLE: i = t                                  ; break;
		default: fprintf( stderr , "[ERROR] FEM::RightTriangle::EvaluateScalarField: unrecognized basis type: %d\n" , BasisType ) , exit( 0 );
	}
	return i * BasisInfo< BasisType >::CoefficientsPerElement + c;
}

template< class Real >
Pointer( FEM::CoordinateXForm< Real > ) FEM::RiemannianMesh< Real >::getCoordinateXForms( void ) const
{
	Pointer( CoordinateXForm< Real > ) xForms = NewPointer< CoordinateXForm< Real > >( _tCount*3 );
	setCoordinateXForms( xForms );
	return xForms;
}
template< class Real >
FEM::CoordinateXForm< Real > FEM::RiemannianMesh< Real >::xForm( unsigned int he ) const
{
	CoordinateXForm< Real > xForm;
	int ohe = _edgeMap.opposite(he);
	if( ohe==-1 ) fprintf( stderr , "[ERROR] Boundary edge (FEM::RiemannianMesh::edgeXForm)\n" ) , exit( 0 );

	// The two triangles on this edge
	int tIdx[] = { he/3 , ohe/3 };

	// The end-points of the edge
	int  _v =  he%3 ,  v[] = { ( he+1)%3 , ( he+2)%3 };
	int _ov = ohe%3 , ov[] = { (ohe+1)%3 , (ohe+2)%3 };

	// The direction of the edge
	TangentVector< Real > edgeDir = RightTriangle< Real >::EdgeDirections[_v] , oEdgeDir = -RightTriangle< Real >::EdgeDirections[_ov];
	// The perpendicular direction to the edge
	TangentVector< Real > edgePerpDir = Rotate90( _g[ tIdx[0] ] , edgeDir ) , oEdgePerpDir = Rotate90( _g[ tIdx[1] ] , oEdgeDir );
		
	// The linear part of the transformation should map ( edgeDir , edgePerpDir ) -> ( oEdgeDir , oEdgePerpDir )
	{
		SquareMatrix< Real , 2 > M , oM;
		M( 0 , 0 ) =     edgeDir[0] , M( 0 , 1 ) =     edgeDir[1];
		M( 1 , 0 ) = edgePerpDir[0] , M( 1 , 1 ) = edgePerpDir[1];
		oM( 0 , 0 ) =     oEdgeDir[0] , oM( 0 , 1 ) =     oEdgeDir[1];
		oM( 1 , 0 ) = oEdgePerpDir[0] , oM( 1 , 1 ) = oEdgePerpDir[1];
		xForm.linear = oM * M.inverse();
	}

	// The transformation should also take ( v[0] + v[1] ) / 2 to ( ov[0] + ov[1] ) / 2
	xForm.constant = RightTriangle< Real >::EdgeMidpoints[_ov] - xForm.linear * ( RightTriangle< Real >::EdgeMidpoints[_v] );
	return xForm;
}

template< class Real >
void FEM::RiemannianMesh< Real >::setCoordinateXForms( Pointer( CoordinateXForm< Real > ) xForms ) const
{
#pragma omp parallel for
	for( int he=0 ; he<_tCount*3 ; he++ ) if( _edgeMap.opposite(he)!=-1 ) xForms[he] = xForm( he );
}
template< class Real >
bool FEM::RiemannianMesh< Real >::edgeFlip( int edge , Real eps )
{
	int he = _edgeMap[edge][0] , ohe = _edgeMap[edge][1];
	int  t =  he / 3 ,  v =  he % 3;
	int ot = ohe / 3 , ov = ohe % 3;

	// First test that the edge is not on the boundary
	if( ohe==-1 ) return false;

	// Get the coordinates of the old and new edges and test that if we can flip
	Point2D< Real > newEdge[] = { RightTriangle< Real >::Corners[v] , edgeXForm( ohe )( RightTriangle< Real >::Corners[ ov ] ) };
	Point2D< Real > oldEdge[] = { RightTriangle< Real >::Corners[(v+1)%3] , RightTriangle< Real >::Corners[(v+2)%3] };
	{

		// Solve for the point of intersection between the two corresponding line segments
		//  => Solve for s and t such that:
		//		newEdge[0] + s * ( newEdge[1]-newEdge[0]) = oldEdge[0] + t * ( oldEdge[1] - oldEdge[0] )
		// <=>	( newEdge[1]-newEdge[0] | -oldEdge[1]+oldEdge[0] ) * (s,t)^t = oldEdge[0] - newEdge[0]
		SquareMatrix< Real , 2 > M;
		for( int i=0 ; i<2 ; i++ ) M(0,i) = newEdge[1][i]-newEdge[0][i] , M(1,i) = -oldEdge[1][i]+oldEdge[0][i];
		Point2D< Real > st = M.inverse() * ( oldEdge[0] - newEdge[0] );

		// Test that the intersection point is in bounds
		if( st[0]<=eps || st[0]>=1-eps || st[1]<=eps || st[1]>=1-eps ) return false;
	}

	//     o              2,1
	//    / \             /|\
	//   /   \           / | \
	//  /     \         /  |  \
	// |-------|  =>  0| o | t |0
	//  \  e  /         \  |  /
	//   \   /           \ | /
	//    \ /             \|/
	//     v              1,2

	//  t*3 + ( v+1) -> ot*3+2
	//  t*3 + ( v+2) ->  t*3+1
	// ot*3 + (ov+1) ->  t*3+2
	// ot*3 + (ov+2) -> ot*3+1

	// The new triangles
	TriangleIndex tris[] = { TriangleIndex( _triangles[t][(v+1)%3] , _triangles[ot][ov] , _triangles[t][v] ) , TriangleIndex( _triangles[t][(v+2)%3] , _triangles[t][v] , _triangles[ot][ov] ) };

	// The new metrics
	SquareMatrix< Real , 2 > tensors[2];
	tensors[0](0,0) = Point2D< Real >::Dot( RightTriangle< Real >::Corners[ov] - RightTriangle< Real >::Corners[(ov+2)%3] , _g[ot] * ( RightTriangle< Real >::Corners[ov] - RightTriangle< Real >::Corners[(ov+2)%3] ) );
	tensors[0](1,1) = Point2D< Real >::Dot( RightTriangle< Real >::Corners[ v] - RightTriangle< Real >::Corners[( v+1)%3] , _g[ t] * ( RightTriangle< Real >::Corners[ v] - RightTriangle< Real >::Corners[( v+1)%3] ) );
	tensors[0](1,0) = tensors[0](0,1) = ( tensors[0](0,0) + tensors[0](1,1) - Point2D< Real >::Dot( newEdge[1]-newEdge[0] , _g[t] * ( newEdge[1]-newEdge[0] ) ) ) / (Real)2.;

	tensors[1](0,0) = Point2D< Real >::Dot( RightTriangle< Real >::Corners[ v] - RightTriangle< Real >::Corners[( v+2)%3] , _g[ t] * ( RightTriangle< Real >::Corners[ v] - RightTriangle< Real >::Corners[( v+2)%3] ) );
	tensors[1](1,1) = Point2D< Real >::Dot( RightTriangle< Real >::Corners[ov] - RightTriangle< Real >::Corners[(ov+1)%3] , _g[ot] * ( RightTriangle< Real >::Corners[ov] - RightTriangle< Real >::Corners[(ov+1)%3] ) );
	tensors[1](1,0) = tensors[1](0,1) = ( tensors[1](0,0) + tensors[1](1,1) - Point2D< Real >::Dot( newEdge[1]-newEdge[0] , _g[t] * ( newEdge[1]-newEdge[0] ) ) ) / (Real)2.;

	int old_he[] = { t*3+(v+1)%3 , ot*3+(ov+1)%3 , t*3+(v+2)%3 , ot*3+(ov+2)%3 } , new_he[] = { 3*ot+2 , 3*t+2 , 3*t+1 , 3*ot+1 };
	int edges[4];
	for( int i=0 ; i<4 ; i++ ) edges[i] = _edgeMap.edge( old_he[i] );
	for( int i=0 ; i<4 ; i++ )
	{
		if     ( _edgeMap._e2he[2*edges[i]+0]==old_he[i] ) _edgeMap._e2he[2*edges[i]+0] = new_he[i] , _edgeMap._he2e[ new_he[i] ] =  edges[i]+1;
		else if( _edgeMap._e2he[2*edges[i]+1]==old_he[i] ) _edgeMap._e2he[2*edges[i]+1] = new_he[i] , _edgeMap._he2e[ new_he[i] ] = -edges[i]-1;
		else fprintf( stderr , "[ERROR] FEM::RiemannianMesh::edgeFlip: Unmatched half-edge\n" ) , exit( 0 );
	}
	_edgeMap._e2he[2*edge+0] = 3* t , _edgeMap._he2e[3* t] =  edge+1;
	_edgeMap._e2he[2*edge+1] = 3*ot , _edgeMap._he2e[3*ot] = -edge-1;

	// Set the triangles and metrics
	_triangles[t] = tris[0] , _triangles[ot] = tris[1];
	_g[t] = tensors[0] , _g[ot] = tensors[1];
	return true;
}

template< class Real >
bool FEM::RiemannianMesh< Real >::isVoronoiEdge( unsigned int e , Real eps ) const
{
	int he = _edgeMap[e][0] , ohe = _edgeMap[e][1];
	int  t =  he / 3 ,  v =  he % 3;
	int ot = ohe / 3 , ov = ohe % 3;
	if( ohe==-1 ) return true;
	Point2D< Real > center = RightTriangle< Real >::Center( _g[t] , RightTriangle< Real >::CENTER_CIRCUMCENTRIC ) , oVertex = edgeXForm( ohe )( RightTriangle< Real >::Corners[ov] );
	return Point2D< Real >::Dot( center - oVertex , _g[t] * ( center - oVertex ) )+eps > Point2D< Real >::Dot( center - RightTriangle< Real >::Corners[0] , _g[t] * ( center - RightTriangle< Real >::Corners[0] ) );
}

template< class Real >
FEM::CoordinateXForm< Real > FEM::RiemannianMesh< Real >::getVertexCoordinateXForm( ConstPointer( CoordinateXForm< Real > ) xForms , int t , int v ) const
{
	const int VertexToEdgeMap[] = { 1 , 2 , 0 };
	const int EdgeToVertexMap[] = { 1 , 2 , 0 };
	// Assume that the mesh is oriented
	FEM::CoordinateXForm< Real > xForm;
	int currentT = t , currentV = v;
	do
	{
		int edge = currentT*3 + VertexToEdgeMap[ currentV ] , oEdge = _edgeMap.opposite( edge );
		xForm = xForms[ edge ] * xForm;
		if( oEdge==-1 ) fprintf( stderr , "[ERROR] Boundary vertex\n" ) , exit( 0 );
		currentT = oEdge / 3;
		currentV = EdgeToVertexMap[ oEdge%3 ];
	}
	while( currentT!=t );
	return xForm;
}

template< class Real >
std::vector< unsigned int > FEM::RiemannianMesh< Real >::getVertexCorners( int t , int v ) const
{
	// Circulate CCW
	const int VertexToEdgeMap[] = { 1 , 2 , 0 };
	const int EdgeToVertexMap[] = { 1 , 2 , 0 };
	// Assume that the mesh is oriented
	std::vector< unsigned int > neighbors;
	int currentT = t , currentV = v;
	do
	{
		unsigned int he = currentT*3 + VertexToEdgeMap[ currentV ] , ohe = _edgeMap.opposite( he );
		neighbors.push_back( currentT*3 + currentV );
		if( ohe==-1 ) fprintf( stderr , "[ERROR] Boundary vertex\n" ) , exit( 0 );
		currentT = ohe / 3;
		currentV = EdgeToVertexMap[ ohe%3 ];
	}
	while( currentT!=t );
	return neighbors;
}
template< class Real >
Real FEM::RiemannianMesh< Real >::getVertexConeAngle( int t , int v ) const
{
	const int VertexToEdgeMap[] = { 1 , 2 , 0 };
	const int EdgeToVertexMap[] = { 1 , 2 , 0 };
	// Assume that the mesh is oriented
	Real angle = (Real)0;
	int currentT = t , currentV = v;
	do
	{
		int he = currentT*3 + VertexToEdgeMap[ currentV ] , ohe = _edgeMap.opposite( he );
		angle += RightTriangle< Real >::Angle( _g[currentT] , currentV );
		if( ohe==-1 ) fprintf( stderr , "[ERROR] Boundary vertex\n" ) , exit( 0 );
		currentT = ohe / 3;
		currentV = EdgeToVertexMap[ ohe%3 ];
	}
	while( currentT!=t );
	return angle;
}

template< class Real >
FEM::CoordinateXForm< Real > FEM::RiemannianMesh< Real >::exp( ConstPointer( CoordinateXForm< Real > ) xForms , HermiteSamplePoint< Real >& p , Real eps ) const
{
	HermiteSamplePoint< Real > startP = p;
	CoordinateXForm< Real > xForm = CoordinateXForm< Real >::Identity();
	if( !Point2D< Real >::SquareNorm( p.v ) ) return xForm;
	const int MAX_ITERS = 10000;
	int count = 0;
	int inEdge = -1;
#if 1
	// If the starting point happens to be on an edge
	{
		int idx = -1;
		if     ( p.p[0]<=0 && p.v[0]<0 ) idx = 1;
		else if( p.p[1]<=0 && p.v[1]<0 ) idx = 2;
		else if( p.p[0]+p.p[1]>=1 && p.v[0]+p.v[1]>0 ) idx = 0;
		if( idx!=-1 )
		{
			unsigned int he = p.tIdx * 3 + idx , ohe = _edgeMap.opposite( he );
			const CoordinateXForm< Real >& edge = xForms[he];
			p.tIdx = ohe/3;
			p.p = edge( p.p ) ; p.v = edge.linear * p.v;
			inEdge = ohe%3;
			xForm = edge * xForm;
		}
	}
#endif
	while( count<MAX_ITERS )
	{
		// Intersect the ray p + s*v with each of the three edges
		// Bottom edge:   p[1] + s * v[1] = 0                         => s = -p[1]/v[1]
		// Left edge:     p[0] + s * v[0] = 0                         => s = -p[0]/v[0]
		// Diagonal edge: p[1] + s * v[1] = 1 - ( p[0] + s * v[0] )   => s = ( 1 - p[0]  - p[1] ) / ( v[1] + v[0] )
		Real maxS = 0;
		int idx = -1;
		{
			Real s[] = { -p.p[1] / p.v[1]  , -p.p[0] / p.v[0] , ( Real(1.) - p.p[0]  - p.p[1] ) / ( p.v[1] + p.v[0] ) };
			if( inEdge!=2 && s[0]>0 ){ Real foo = p.p[0] + p.v[0] * s[0] ; if( foo>=-eps && foo<=1+eps ) if( s[0]>maxS ) idx = 2 , maxS = s[0]; }
			if( inEdge!=1 && s[1]>0 ){ Real foo = p.p[1] + p.v[1] * s[1] ; if( foo>=-eps && foo<=1+eps ) if( s[1]>maxS ) idx = 1 , maxS = s[1]; }
			if( inEdge!=0 && s[2]>0 ){ Real foo = p.p[0] + p.v[0] * s[2] ; if( foo>=-eps && foo<=1+eps ) if( s[2]>maxS ) idx = 0 , maxS = s[2]; }
		}
		if( idx==-1 )
		{
			/*fprintf( stderr , "[ERROR] FEM::Mesh::exp:\n" );
			fprintf( stderr , "        Ray does not intersect triangle[%d]: p=(%f %f) v=(%g %g) [%g/%g]\n" , count , p.p[0] , p.p[1] , p.v[0] , p.v[1] , Point2D< Real >::SquareNorm(p.v) , eps*eps );
			fprintf( stderr , "                             Started at[%d]: p=(%f %f) v=(%g %g)\n" , 0 , startP.p[0] , startP.p[1] , startP.v[0] , startP.v[1] );*/
			p = startP;
			return xForm;
			//exit( 0 );
		}
		if( maxS>1 ) // The end-point is within the triangle
		{
			p.p += p.v , p.v -= p.v;
			return xForm;
		}
		else // The end-point is outside the triangle
		{
			unsigned int he = p.tIdx*3 + idx , ohe = _edgeMap.opposite( he );
			const CoordinateXForm< Real >& edge = xForms[he];
			p.p += p.v*maxS ; p.v -= p.v*maxS ; p.tIdx = ohe/3;
			p.p = edge( p.p ) ; p.v = edge.linear * p.v;
			inEdge = ohe%3;
			xForm = edge * xForm;
		}
		count++;
	}
	fprintf( stderr , "[WARNING] Failed to converge exp after %d iterations\n" , MAX_ITERS );
	return xForm;
}

template< class Real >
FEM::CoordinateXForm< Real > FEM::RiemannianMesh< Real >::flow( ConstPointer( CoordinateXForm< Real > ) xForms , const TangentVectorField< Real >& vf , Real flowTime , SamplePoint< Real >& p , Real minStepSize , Real eps , std::vector< SamplePoint< Real > >* path ) const
{
	CoordinateXForm< Real > xForm = CoordinateXForm< Real >::Identity();
	int MAX_ITERS = 1000000;
	int count = 0;
	int inEdge = -1;
	Real direction = (flowTime<0) ? (Real)-1. : (Real)1.;
	Real stepSizeLeft = minStepSize;
	Point2D< Real > v = vf( p ) * direction;
	Real vLength = (Real)sqrt( Point2D< Real >::Dot( v , _g[p.tIdx]*v ) );
	flowTime *= direction;
	if( path ) path->push_back( p );
	while( count<MAX_ITERS )
	{
		if( !Point2D< Real >::SquareNorm( v ) ) return xForm;
		// Intersect the ray p + s * v with each of the three edges
		// Bottom edge:   p[1] + s * v[1] = 0                         => s = -p[1]/v[1]
		// Left edge:     p[0] + s * v[0] = 0                         => s = -p[0]/v[0]
		// Diagonal edge: p[1] + s * v[1] = 1 - ( p[0] + s * v[0] )   => s = ( 1 - p[0]  - p[1] ) / ( v[1] + v[0] )
		Real s = 0;
		int idx = -1;
		{
#if 1
			Real _s[] = { -p.p[1] / v[1]  , -p.p[0] / v[0] , ( Real(1.) - p.p[0]  - p.p[1] ) / ( v[1] + v[0] ) };
			s = 1e10;
			if( inEdge!=2 && _s[0]>0 && _s[0]<s ) idx = 2 , s = _s[0];
			if( inEdge!=1 && _s[1]>0 && _s[1]<s ) idx = 1 , s = _s[1];
			if( inEdge!=0 && _s[2]>0 && _s[2]<s ) idx = 0 , s = _s[2];
#else
			if( inEdge!=2 && _s[0]>0 ){ Real foo = p.p[0] + v[0] * _s[0] ; if( foo>=-eps && foo<=1+eps ) if( _s[0]>s ) idx = 2 , s = _s[0]; }
			if( inEdge!=1 && _s[1]>0 ){ Real foo = p.p[1] + v[1] * _s[1] ; if( foo>=-eps && foo<=1+eps ) if( _s[1]>s ) idx = 1 , s = _s[1]; }
			if( inEdge!=0 && _s[2]>0 ){ Real foo = p.p[0] + v[0] * _s[2] ; if( foo>=-eps && foo<=1+eps ) if( _s[2]>s ) idx = 0 , s = _s[2]; }
#endif
		}
#if 0
		if( idx==-1 )
		{
			fprintf( stderr , "[ERROR] Ray does not intersect triangle[%d]: (%f %f) (%g %g) [%g/%g]\n" , count , p.p[0] , p.p[1] , v[0] , v[1] , Point2D< Real >::SquareNorm(v) , eps*eps );
			Real s[] = { -p.p[1] / v[1]  , -p.p[0] / v[0] , ( Real(1.) - p.p[0]  - p.p[1] ) / ( v[1] + v[0] ) };
			if( inEdge!=2 ) { Real foo = p.p[0] + v[0] * s[0] ; printf( "\t0] %g -> %f\n" , s[0] , foo ); }
			if( inEdge!=1 ) { Real foo = p.p[1] + v[1] * s[1] ; printf( "\t1] %g -> %f\n" , s[1] , foo ); }
			if( inEdge!=0 ) { Real foo = p.p[0] + v[0] * s[2] ; printf( "\t2] %g -> %f\n" , s[2] , foo ); }
			exit( 0 );
		}
#else
		if( idx==-1 ) return xForm;
#endif
		Point2D< Real > gv = _g[p.tIdx] * v;
		Real stepSize = vLength * s;
		bool updateVector = false;
		if( minStepSize>0 && stepSize>stepSizeLeft )
		{
			s = stepSizeLeft / vLength;
			updateVector = true;
		}

		// If we can finish the flow
		if( flowTime<s )
		{
			p.p += v * flowTime;
			if( path ) path->push_back( p );
			return xForm;
		}
		else
		{
			p.p += v * s , flowTime -= s;

			// If we do not cross a boundary, change direction
			if( updateVector )
			{
				// If the the vectors are oppositely oriented, terminate the flow
				if( Point2D< Real >::Dot( gv , vf( p ) )*direction < 0 ) return xForm;

				v = vf( p ) * direction;
				vLength = (Real)sqrt( Point2D< Real >::Dot( v , _g[p.tIdx]*v ) );
				stepSizeLeft = minStepSize;
				inEdge = -1;
			}
			// If we cross the boundary, transport the direction of the flow
			else // The end-point is outside the triangle
			{
				// Advance along the flow until you hit the edge	
				unsigned int he = p.tIdx*3 + idx , ohe = _edgeMap.opposite( he );
				const CoordinateXForm< Real >& edge = xForms[he];
				// Switch into the next triangle
				p.tIdx = ohe/3;
				p.p = edge( p.p );
				v = edge.linear * v;

				// Mark the edge we came in on
				inEdge = ohe%3;

				// Accumulate the transformations
				xForm = edge * xForm;

				stepSizeLeft -= stepSize;
			}

			if( path ) path->push_back( p );
		}
		count++;
	}
	fprintf( stderr , "[WARNING] Failed to converge flow after %d iterations\n" , MAX_ITERS );
	return xForm;
#undef NEW_CODE
}
template< class Real >
Real FEM::RiemannianMesh< Real >::flow( ConstPointer( CoordinateXForm< Real > ) xForms , const TangentVectorField< Real >& vf , Real flowTime , SamplePoint< Real >& p , FEM::CoordinateXForm< Real >& xForm , Real minFlowTime , Real eps ) const
{
	Real distance = (Real)0;
#define DEBUG_FLOW 0
	xForm = CoordinateXForm< Real >::Identity();
#if DEBUG_FLOW
	const int MAX_ITERS = 10;
#else // !DEBUG_FLOW
	const int MAX_ITERS = 1000000;
#endif // DEBUG_FLOW
	int count = 0;
	int inEdge = -1;
	Point2D< Real > v = (*vf)( p );
	Real direction = (flowTime<0) ? (Real)-1. : (Real)1.;
	flowTime *= direction;
	while( count<MAX_ITERS )
	{
		v *= direction;
		if( !Point2D< Real >::SquareNorm( v ) ) return distance;
		// Intersect the ray p + s*v with each of the three edges
		// Bottom edge:   p[1] + s * v[1] = 0                         => s = -p[1]/v[1]
		// Left edge:     p[0] + s * v[0] = 0                         => s = -p[0]/v[0]
		// Diagonal edge: p[1] + s * v[1] = 1 - ( p[0] + s * v[0] )   => s = ( 1 - p[0]  - p[1] ) / ( v[1] + v[0] )
		Real maxD = 0;
		int idx = -1;
		{
			Real s[] = { -p.p[1] / v[1]  , -p.p[0] / v[0] , ( Real(1.) - p.p[0]  - p.p[1] ) / ( v[1] + v[0] ) };
			if( inEdge!=2 && s[0]>0 ){ Real foo = p.p[0] + v[0] * s[0] ; if( foo>=-eps && foo<=1+eps ) if( s[0]>maxD ) idx = 2 , maxD = s[0]; }
			if( inEdge!=1 && s[1]>0 ){ Real foo = p.p[1] + v[1] * s[1] ; if( foo>=-eps && foo<=1+eps ) if( s[1]>maxD ) idx = 1 , maxD = s[1]; }
			if( inEdge!=0 && s[2]>0 ){ Real foo = p.p[0] + v[0] * s[2] ; if( foo>=-eps && foo<=1+eps ) if( s[2]>maxD ) idx = 0 , maxD = s[2]; }
		}
#if 0
		if( idx==-1 )
		{
			fprintf( stderr , "[ERROR] Ray does not intersect triangle[%d]: (%f %f) (%g %g) [%g/%g]\n" , count , p.p[0] , p.p[1] , v[0] , v[1] , Point2D< Real >::SquareNorm(v) , eps*eps );
			Real s[] = { -p.p[1] / v[1]  , -p.p[0] / v[0] , ( Real(1.) - p.p[0]  - p.p[1] ) / ( v[1] + v[0] ) };
			if( inEdge!=2 ) { Real foo = p.p[0] + v[0] * s[0] ; printf( "\t0]%g -> %f\n" , s[0] , foo ); }
			if( inEdge!=1 ) { Real foo = p.p[1] + v[1] * s[1] ; printf( "\t1]%g -> %f\n" , s[1] , foo ); }
			if( inEdge!=0 ) { Real foo = p.p[0] + v[0] * s[2] ; printf( "\t2]%g -> %f\n" , s[2] , foo ); }
			exit( 0 );
		}
#else
		Real vLength = (Real)sqrt( Point2D< Real >::Dot( v , _g[p.tIdx] * v ) );
		if( idx==-1 ) return distance;
#endif
#if DEBUG_FLOW
		printf( "maxD[%d] %g\n" , count , maxD );
#endif // DEBUG_FLOW
		if( maxD>flowTime ) // The end-point is within the triangle
		{
			distance += vLength * flowTime;
			p.p += v*flowTime;
			return distance;
		}
		else // The end-point is outside the triangle
		{
			unsigned int he = p.tIdx*3 + idx , ohe = _edgeMap.opposite( he );
			const CoordinateXForm< Real >& edge = xForms[he];

			// If the the vectors on the two sides of the edge are oppositely oriented, terminate the flow
			if( Point2D< Real >::Dot( edge.linear * v , _g[ohe/3] * vf[ohe/3] )*direction < 0 ) return distance;
			distance += vLength * maxD;
			p.p += v*maxD , p.tIdx = ohe/3 , flowTime -= maxD;
			p.p = edge( p.p );
			v = vf[ p.tIdx ];
			inEdge = ohe%3;

			xForm = edge * xForm;
		}
		count++;
	}
	fprintf( stderr , "[WARNING] Failed to converge flow after %d iterations\n" , MAX_ITERS );
	return distance;
#undef DEBUG_FLOW
}

/////////////////////////
// FEM::RiemannianMesh //
/////////////////////////
template< class Real >
inline void FEM::RiemannianMesh< Real >::makeUnitArea( void )
{
	double scale = 0;
#pragma omp parallel for reduction( + : scale )
	for( int i=0 ; i<_tCount ; i++ ) scale += sqrt( _g[i].determinant() );
	scale = 2. / scale;
#pragma omp parallel for
	for( int i=0 ; i<_tCount ; i++ ) _g[i] *= (Real)scale;
}
template< class Real >
inline Real FEM::RiemannianMesh< Real >::area( void ) const
{
	Real area = 0;
#pragma omp parallel for reduction( + : area )
	for( int i=0 ; i<_tCount ; i++ ) area += (Real)sqrt( _g[i].determinant() );
	return area / (Real)2.;
}
template< class Real >
inline Real FEM::RiemannianMesh< Real >::area( int idx ) const { return (Real)sqrt( _g[idx].determinant() ) / (Real)2.; }

template< class Real >
inline Real FEM::RiemannianMesh< Real >::squareEdgeLength( int heIdx ) const { return SquareLength( _g[heIdx/3] , RightTriangle< Real >::EdgeDirections[heIdx%3] ); }

template< class Real >
template< class Vertex >
void FEM::RiemannianMesh< Real >::setMetricFromEmbedding( ConstPointer( Vertex ) vertices )
{
	bool vanishingTensor = false;
#pragma omp parallel for
	for( int i=0 ; i<_tCount ; i++ )
	{
		Point3D< Real > e[] = { Point3D< Real >( vertices[ _triangles[i][1] ] ) - Point3D< Real >( vertices[ _triangles[i][0] ] ) , Point3D< Real >( vertices[ _triangles[i][2] ] ) - Point3D< Real >( vertices[ _triangles[i][0] ] ) };
		for( int j=0 ; j<2 ; j++ ) for( int k=0 ; k<2 ; k++ ) _g[i](j,k) = Point3D< Real >::Dot( e[j] , e[k] );
		_g[i](0,1) = _g[i](1,0) = (Real)( ( _g[i](0,1) + _g[i](1,0) )/2 );

		if( !_g[i].determinant() )
		{
			vanishingTensor = true;
			fprintf( stderr , "[WARNING] Vanishing metric tensor determinant\n" );
			printf( "%g %g %g\t%g %g %g\n" , e[0][0] , e[0][1] , e[0][2] , e[1][0] , e[1][1] , e[1][2] );
		}
	}
	if (vanishingTensor){
		fprintf(stderr, "[WARNING] Adding regularization to metric tensor\n");
		Real average_squared_edge_length = Real(0);
		for (int i = 0; i < _tCount; i++) for (int j = 0; j < 3; j++) average_squared_edge_length += Point3D< Real >::SquareNorm(vertices[_triangles[i][(j + 1) % 3]] - vertices[_triangles[i][j]]);
		average_squared_edge_length /= Real(3 * _tCount);
		printf("Average Squared Edge Length %g \n", average_squared_edge_length);
		for (int i = 0; i < _tCount; i++)
		{
			_g[i](0, 0) += average_squared_edge_length * 0.001;
			_g[i](1, 1) += average_squared_edge_length * 0.001;
		}
	}
}
template< class Real >
void FEM::RiemannianMesh< Real >::setMetricFromEdgeLengths( ConstPointer( Real ) edgeLengths )
{
#pragma omp parallel for
	for( int i=0 ; i<_tCount ; i++ )
	{
		// The conditions the matrix needs to satisfy are:
		// -- < g[i] * (1,0) , (1,0) > = edgeLengths[i*3+2]^2
		//		=> g[i](0,0) = edgeLengths[i*3+2]^2
		// -- < g[i] * (0,1) , (0,1) > = edgeLengths[i*3+1]^2
		//		=> g[i](1,1) = edgeLengths[i*3+1]^2
		// -- < g[i] * (-1,1) , (-1,1) > = edgeLengths[i*3+0]^2
		//		=> g[i](0,0) + g[i](1,1) - g[i](0,1) - g[i](1,0) = edgeLengths[i*3+0]^2
		//		=> - g[i](0,1) - g[i](1,0) = edgeLengths[i*3+0]^2 - edgeLengths[i*3+2]^2 - edgeLengths[i*3+1]^2
		//		=>  g[i](0,1) = g[i](1,0) = ( edgeLengths[i*3+2]^2 + edgeLengths[i*3+1]^2 - edgeLengths[i*3+0]^2 ) / 2

		_g[i](0,0) = edgeLengths[i*3+2] * edgeLengths[i*3+2];
		_g[i](1,1) = edgeLengths[i*3+1] * edgeLengths[i*3+1];
		_g[i](0,1) = _g[i](1,0) = ( _g[i](0,0) + _g[i](1,1) - edgeLengths[i*3] * edgeLengths[i*3] ) / (Real)2.;
	}
}
template< class Real >
void FEM::RiemannianMesh< Real >::setMetricFromSquareEdgeLengths( ConstPointer( Real ) squareEdgeLengths )
{
#pragma omp parallel for
	for( int i=0 ; i<tCount ; i++ )
	{
		_g[i](0,0) = squareEdgeLengths[i*3+2];
		_g[i](1,1) = squareEdgeLengths[i*3+1];
		_g[i](0,1) = _g[i](1,0) = ( _g[i](0,0) + _g[i](1,1) - squareEdgeLengths[i*3] ) / (Real)2.;
	}
}
template< class Real >
template< unsigned int BasisType , class V >
V FEM::RiemannianMesh< Real >::evaluateScalarField( ConstPointer( V ) coefficients , const SamplePoint< Real >& p ) const
{
	V v;
	TestBasisType( BasisType , "FEM::RiemannianMesh::evaluateScalarField" , false );
	if( BasisInfo< BasisType >::Dimension==0 )
	{
		V _coefficients[ BasisInfo< BasisType >::Coefficients ];
		for( int i=0 ; i<BasisInfo< BasisType >::Coefficients ; i++ )
		{
			bool isAligned;
			int ii = index< BasisType >( p.tIdx , i , isAligned );
			_coefficients[i] = isAligned ? coefficients[ii] : -coefficients[ii];
		}
		v = RightTriangle< Real >::EvaluateScalarField< BasisType >( ( ConstPointer( V ) )GetPointer( _coefficients , BasisInfo< BasisType >::Coefficients ) , p.p );
	}
	else TestBasisType( BasisType , "FEM::RiemannianMesh::evaluateScalarField" , false );
	return v;
}
template< class Real >
template< unsigned int BasisType , class V >
FEM::CotangentVector< V > FEM::RiemannianMesh< Real >::evaluateScalarFieldGradient( ConstPointer( V ) coefficients , const SamplePoint< Real >& p ) const
{
	CotangentVector< V > v;
	TestBasisType( BasisType , "FEM::RiemannianMesh::evaluateScalarField" , false );
	if( BasisInfo< BasisType >::Dimension==0 )
	{
		Real _coefficients[ BasisInfo< BasisType >::Coefficients ];
		for( int i=0 ; i<BasisInfo< BasisType >::Coefficients ; i++ )
		{
			bool isAligned;
			int ii = index< BasisType >( p.tIdx , i , isAligned );
			_coefficients[i] = isAligned ? coefficients[ii] : -coefficients[ii];
		}
		v = RightTriangle< Real >::EvaluateScalarFieldGradient< BasisType >( GetPointer( _coefficients , BasisInfo< BasisType >::Coefficients ) , p.p );
	}
	else TestBasisType( BasisType , "FEM::RiemannianMesh::evaluateScalarFieldGradient" , false );
	return v;
}
template< class Real >
template< unsigned int BasisType , class V >
FEM::CotangentVector< V > FEM::RiemannianMesh< Real >::evaluateCovectorField( ConstPointer( V ) coefficients , const SamplePoint< Real >& p ) const
{
	CotangentVector< V > v;
	TestBasisType( BasisType , "FEM::RiemannianMesh::evaluateCovectorField" , false );
	if( BasisInfo< BasisType >::Dimension==1 )
	{
		Real _coefficients[ BasisInfo< BasisType >::Coefficients ];
		for( int i=0 ; i<BasisInfo< BasisType >::Coefficients ; i++ )
		{
			bool isAligned;
			int ii = index< BasisType >( p.tIdx , i , isAligned );
			_coefficients[i] = isAligned ? coefficients[ii] : -coefficients[ii];
		}
		v = RightTriangle< Real >::EvaluateCovectorField< BasisType >( GetPointer( _coefficients , BasisInfo< BasisType >::Coefficients ) , p.p );
	}
	else TestBasisType( BasisType , "FEM::RiemannianMesh::evaluateScalarField" , false );
	return v;
}
template< class Real >
template< unsigned int BasisType >
SparseMatrix< Real , int > FEM::RiemannianMesh< Real >::massMatrix( bool lump , ConstPointer( SquareMatrix< Real , 2 > ) newTensors ) const
{
	SparseMatrix< Real , int > M;
	if( BasisType==BASIS_2_VERTEX_CONSTANT )
	{
		M.resize( _vCount );
#pragma omp parallel for
		for( int i=0 ; i<_vCount ; i++ ) M.SetRowSize( i , 1 ) , M[i][0] = MatrixEntry< Real , int >( i , 0 );
#pragma omp parallel for
		for( int i=0 ; i<_tCount ; i++ )
		{
			Real a = area( i ) / (Real)3.;
			if( newTensors ) a *= newTensors[i].determinant() / _g[i].determinant() / _g[i].determinant();
			for( int j=0 ; j<3 ; j++ )
#pragma omp atomic
				M[ _triangles[i][j] ][0].Value += a;
		}
#pragma omp parallel for
		for( int i=0 ; i<M.rows ; i++ ) M[i][0].Value = (Real)( 1. / M[i][0].Value );
		return M;
	}
	SquareMatrix< unsigned char , BasisInfo< BasisType >::Coefficients > mask = RightTriangle< Real >::GetMassMask< BasisType >( newTensors!=NullPointer< SquareMatrix< Real , 2 > >() );
	Point< int , BasisInfo< BasisType >::Coefficients > nonZeroCount;
	for( int i=0 ; i<BasisInfo< BasisType >::Coefficients ; i++ ) for( int j=0 ; j<BasisInfo< BasisType >::Coefficients ; j++ ) if( mask(i,j) ) nonZeroCount[i]++;

	M.resize( dimension< BasisType >() );
	Pointer( std::atomic< int > ) rowSizes = NewPointer< std::atomic< int > >( M.rows ); // need to support atomic increment and set, which is not supported with OpenMP
#pragma omp parallel for
	for( int i=0 ; i<M.rows ; i++ ) rowSizes[i] = 0;

	// First, set the row sizes
#pragma omp parallel for
	for( int t=0 ; t<_tCount ; t++ )
		if( lump ) for( int i=0 ; i<BasisInfo< BasisType >::Coefficients ; i++ ) rowSizes[ index< BasisType >( t , i ) ]++;
		else for( int i=0 ; i<BasisInfo< BasisType >::Coefficients ; i++ ) rowSizes[ index< BasisType >( t , i ) ] += nonZeroCount[i];
#pragma omp parallel for
	for( int i=0 ; i<M.rows ; i++ ) M.SetRowSize( i , rowSizes[i] ) , rowSizes[i] = 0;

	// Next, set the entries
#pragma omp parallel for
	for( int t=0 ; t<_tCount ; t++ )
	{
		if( lump )
		{
			auto m = RightTriangle< Real >::GetDiagonalMassMatrix< BasisType >( _g[t] );
			for( int i=0 ; i<BasisInfo< BasisType >::Coefficients ; i++ )
			{
				int ii = index< BasisType >( t , i );
				M[ ii ][ rowSizes[ii]++ ] = MatrixEntry< Real , int >( ii , m[i] );
			}
		}
		else
		{
			auto m = newTensors ? RightTriangle< Real >::GetMassMatrix< BasisType >( _g[t] , newTensors[t] ) : RightTriangle< Real >::GetMassMatrix< BasisType >( _g[t] );
			for( int i=0 ; i<BasisInfo< BasisType >::Coefficients ; i++ ) 
			{
				bool iAligned;
				int ii = index< BasisType >( t , i , iAligned );

				int idx = 0;
				for( int j=0 ; j<BasisInfo< BasisType >::Coefficients ; j++ ) if( mask(i,j) )
				{
					bool jAligned;
					int jj = index< BasisType >( t , j , jAligned );
					M[ ii ][ rowSizes[ii]++ ] = MatrixEntry< Real , int >( jj , iAligned==jAligned ? m(i,j) : - m(i,j) );
				}
			}
		}
	}
	DeletePointer( rowSizes );

	// Collapse the duplicate entries (and sort)
#pragma omp parallel for
	for( int i=0 ; i<M.rows ; i++ )
	{
		std::sort( M[i] , M[i] + M.rowSizes[i] , []( MatrixEntry< Real , int > e1 , MatrixEntry< Real , int > e2 ){ return e1.N<e2.N; } );
		int idx = 0;
		for( int j=1 ; j<M.rowSizes[i] ; j++ )
			if( M[i][j].N==M[i][idx].N ) M[i][idx].Value += M[i][j].Value;
			else M[i][++idx] = M[i][j];
		M.ResetRowSize( i , idx+1 );
		for( int j=0 ; j<M.rowSizes[i] ; j++ ) if( M[i][j].N==i ) std::swap( M[i][j] , M[i][0] );
	}
	return M;
}

template< class Real >
inline Real FEM::RiemannianMesh< Real >::getIntegral( ConstPointer( Real ) coefficients ) const
{
	Real integral = (Real)0;
#pragma omp parallel for
	for( int i=0 ; i<_tCount ; i++ )
	{
		SquareMatrix< Real , 3 > mass = FEM::RightTriangle< Real >::GetMassMatrix< FEM::BASIS_0_WHITNEY >( _g[i] );
		for( int j=0 ; j<3 ; j++ )
		{
			Real sum = (Real)0;
			for( int k=0 ; k<3 ; k++ ) sum += mass(j,k);
#pragma omp atomic
			integral += coefficients[ _triangles[i][j] ] * sum;
		}
	}
	return integral;
}

FEM::EdgeMap::EdgeMap( ConstPointer( TriangleIndex ) triangles , size_t tCount )
{
	_eCount = 0;
	std::unordered_map< long long , int > edgeMap;
	for( int t=0 ; t<tCount ; t++ ) for( int v=0 ; v<3 ; v++ )
	{
		long long key = EdgeKey( triangles[t][(v+1)%3] , triangles[t][(v+2)%3] );
		if( edgeMap.find(key)==edgeMap.end() ) edgeMap[key] = _eCount++;
	}
	_he2e = AllocPointer< int >( tCount*3 );
	_e2he = AllocPointer< int >( 2*_eCount );
	memset( _e2he , -1 , sizeof(int) * 2 * _eCount );

	for( int t=0 ; t<tCount ; t++ ) for( int v=0 ; v<3 ; v++ )
	{
		long long key = EdgeKey( triangles[t][(v+1)%3] , triangles[t][(v+2)%3] );
		int heIdx = t*3+v , eIdx = edgeMap[key];
		if( _e2he[2*eIdx]==-1 ) _e2he[eIdx*2+0] = heIdx , _he2e[heIdx] =  (eIdx+1);
		else                    _e2he[eIdx*2+1] = heIdx , _he2e[heIdx] = -(eIdx+1);
	}
}