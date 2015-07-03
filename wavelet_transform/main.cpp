#include <iostream>
#include <opencv.hpp>
using namespace std;

using namespace cv;
#define THID_ERR_NONE 1
#define PATH "image.jpg"

////////////////////////////////////////////////////////////////////////////////////////////

/// ���ú���

/// ���ɲ�ͬ���͵�С��������ֻ��haar��sym2
void wavelet( const string _wname, Mat &_lowFilter, Mat &_highFilter )
{
	if ( _wname=="haar" || _wname=="db1" )
	{
		int N = 2;
		_lowFilter = Mat::zeros( 1, N, CV_32F );
		_highFilter = Mat::zeros( 1, N, CV_32F );

		_lowFilter.at<float>(0, 0) = 1/sqrtf(N); 
		_lowFilter.at<float>(0, 1) = 1/sqrtf(N); 

		_highFilter.at<float>(0, 0) = -1/sqrtf(N); 
		_highFilter.at<float>(0, 1) = 1/sqrtf(N); 
	}
	if ( _wname =="sym2" )
	{
		int N = 4;
		float h[] = {-0.483, 0.836, -0.224, -0.129 };
		float l[] = {-0.129, 0.224,    0.837, 0.483 };

		_lowFilter = Mat::zeros( 1, N, CV_32F );
		_highFilter = Mat::zeros( 1, N, CV_32F );

		for ( int i=0; i<N; i++ )
		{
			_lowFilter.at<float>(0, i) = l[i]; 
			_highFilter.at<float>(0, i) = h[i]; 
		}

	}
}

/// С���ֽ�
Mat waveletDecompose( const Mat &_src, const Mat &_lowFilter, const Mat &_highFilter )
{
	assert( _src.rows==1 && _lowFilter.rows==1 && _highFilter.rows==1 );
	assert( _src.cols>=_lowFilter.cols && _src.cols>=_highFilter.cols );
	Mat &src = Mat_<float>(_src);

	int D = src.cols;

	Mat &lowFilter = Mat_<float>(_lowFilter);
	Mat &highFilter = Mat_<float>(_highFilter);


	/// Ƶ���˲�����ʱ������ifft( fft(x) * fft(filter)) = cov(x,filter) 
	Mat dst1 = Mat::zeros( 1, D, src.type() );
	Mat dst2 = Mat::zeros( 1, D, src.type()  );

	filter2D( src, dst1, -1, lowFilter );
	filter2D( src, dst2, -1, highFilter );


	/// �²���
	Mat downDst1 = Mat::zeros( 1, D/2, src.type() );
	Mat downDst2 = Mat::zeros( 1, D/2, src.type() );

	resize( dst1, downDst1, downDst1.size() );
	resize( dst2, downDst2, downDst2.size() );


	/// ����ƴ��
	for ( int i=0; i<D/2; i++ )
	{
		src.at<float>(0, i) = downDst1.at<float>( 0, i );
		src.at<float>(0, i+D/2) = downDst2.at<float>( 0, i );
	}

	return src;
}

/// С���ؽ�
Mat waveletReconstruct( const Mat &_src, const Mat &_lowFilter, const Mat &_highFilter )
{
	assert( _src.rows==1 && _lowFilter.rows==1 && _highFilter.rows==1 );
	assert( _src.cols>=_lowFilter.cols && _src.cols>=_highFilter.cols );
	Mat &src = Mat_<float>(_src);

	int D = src.cols;

	Mat &lowFilter = Mat_<float>(_lowFilter);
	Mat &highFilter = Mat_<float>(_highFilter);

	/// ��ֵ;
	Mat Up1 = Mat::zeros( 1, D, src.type() );
	Mat Up2 = Mat::zeros( 1, D, src.type() );

	/// ��ֵΪ0
	//for ( int i=0, cnt=1; i<D/2; i++,cnt+=2 )
	//{
	//    Up1.at<float>( 0, cnt ) = src.at<float>( 0, i );     ///< ǰһ��
	//    Up2.at<float>( 0, cnt ) = src.at<float>( 0, i+D/2 ); ///< ��һ��
	//}

	/// ���Բ�ֵ
	Mat roi1( src, Rect(0, 0, D/2, 1) );
	Mat roi2( src, Rect(D/2, 0, D/2, 1) );
	resize( roi1, Up1, Up1.size(), 0, 0, INTER_CUBIC );
	resize( roi2, Up2, Up2.size(), 0, 0, INTER_CUBIC );

	/// ǰһ���ͨ����һ���ͨ
	Mat dst1 = Mat::zeros( 1, D, src.type() );
	Mat dst2= Mat::zeros( 1, D, src.type() );
	filter2D( Up1, dst1, -1, lowFilter );
	filter2D( Up2, dst2, -1, highFilter );

	/// ������
	dst1 = dst1 + dst2;

	return dst1;

}

///  С���任
Mat WDT( const Mat &_src, const string _wname, const int _level )
{
	int reValue = THID_ERR_NONE;
	Mat src = Mat_<float>(_src);
	Mat dst = Mat::zeros( src.rows, src.cols, src.type() ); 
	int N = src.rows;
	int D = src.cols;

	/// ��ͨ��ͨ�˲���
	Mat lowFilter; 
	Mat highFilter;
	wavelet( _wname, lowFilter, highFilter );

	/// С���任
	int t=1;
	int row = N;
	int col = D;

	while( t<=_level )
	{
		///�Ƚ�����С���任
		for( int i=0; i<row; i++ ) 
		{
			/// ȡ��src��Ҫ��������ݵ�һ��
			Mat oneRow = Mat::zeros( 1,col, src.type() );
			for ( int j=0; j<col; j++ )
			{
				oneRow.at<float>(0,j) = src.at<float>(i,j);
			}
			oneRow = waveletDecompose( oneRow, lowFilter, highFilter );
			/// ��src��һ����ΪoneRow�е�����
			for ( int j=0; j<col; j++ )
			{
				dst.at<float>(i,j) = oneRow.at<float>(0,j);
			}
		}

#if 0
		//normalize( dst, dst, 0, 255, NORM_MINMAX );
		IplImage dstImg1 = IplImage(dst); 
		cvSaveImage( "dst.jpg", &dstImg1 );
#endif
		/// С���б任
		for ( int j=0; j<col; j++ )
		{
			/// ȡ��src���ݵ�һ������
			Mat oneCol = Mat::zeros( row, 1, src.type() );
			for ( int i=0; i<row; i++ )
			{
				oneCol.at<float>(i,0) = dst.at<float>(i,j);
			}
			oneCol = ( waveletDecompose( oneCol.t(), lowFilter, highFilter ) ).t();

			for ( int i=0; i<row; i++ )
			{
				dst.at<float>(i,j) = oneCol.at<float>(i,0);
			}
		}

#if 0
		normalize( dst, dst, 0, 255, NORM_MINMAX );
		IplImage dstImg2 = IplImage(dst); 
		cvSaveImage( "dst.jpg", &dstImg2 );
#endif

		/// ����
		row /= 2;
		col /=2;
		t++;
		src = dst;
	}

	return dst;
}

///  С����任
Mat IWDT( const Mat &_src, const string _wname, const int _level )
{
	int reValue = THID_ERR_NONE;
	Mat src = Mat_<float>(_src);
	Mat dst = Mat::zeros( src.rows, src.cols, src.type() ); 
	int N = src.rows;
	int D = src.cols;

	/// ��ͨ��ͨ�˲���
	Mat lowFilter; 
	Mat highFilter;
	wavelet( _wname, lowFilter, highFilter );

	/// С���任
	int t=1;
	int row = N/std::pow( 2., _level-1);
	int col = D/std::pow(2., _level-1);

	while ( row<=N && col<=D )
	{
		/// С������任
		for ( int j=0; j<col; j++ )
		{
			/// ȡ��src���ݵ�һ������
			Mat oneCol = Mat::zeros( row, 1, src.type() );
			for ( int i=0; i<row; i++ )
			{
				oneCol.at<float>(i,0) = src.at<float>(i,j);
			}
			oneCol = ( waveletReconstruct( oneCol.t(), lowFilter, highFilter ) ).t();

			for ( int i=0; i<row; i++ )
			{
				dst.at<float>(i,j) = oneCol.at<float>(i,0);
			}
		}

#if 0
		//normalize( dst, dst, 0, 255, NORM_MINMAX );
		IplImage dstImg2 = IplImage(dst); 
		cvSaveImage( "dst.jpg", &dstImg2 );
#endif
		///��С����任
		for( int i=0; i<row; i++ ) 
		{
			/// ȡ��src��Ҫ��������ݵ�һ��
			Mat oneRow = Mat::zeros( 1,col, src.type() );
			for ( int j=0; j<col; j++ )
			{
				oneRow.at<float>(0,j) = dst.at<float>(i,j);
			}
			oneRow = waveletReconstruct( oneRow, lowFilter, highFilter );
			/// ��src��һ����ΪoneRow�е�����
			for ( int j=0; j<col; j++ )
			{
				dst.at<float>(i,j) = oneRow.at<float>(0,j);
			}
		}

#if 0
		//normalize( dst, dst, 0, 255, NORM_MINMAX );
		IplImage dstImg1 = IplImage(dst); 
		cvSaveImage( "dst.jpg", &dstImg1 );
#endif

		row *= 2;
		col *= 2;
		src = dst;
	}

	return dst;
}




int main()
{
	Mat image=imread(PATH);
	if (!image.data)
	{
		return 0;
	}
	int W=image.cols;
	int H=image.rows;
	Mat gray;
	cvtColor(image,gray,CV_BGR2GRAY);

	if (W/2!=0||H/2!=0)
	{
		W=(W/2)*2;
		H=(H/2)*2;
	}
	resize(gray,gray,Size(W,H),0,0,1);
	namedWindow("lena",1);
	imshow("lena",gray);
	Mat src;
	gray.convertTo(src,CV_32F);
	Mat dst=WDT(src,"sym2",1);

	/*namedWindow("WDT",1);
	imshow("WDT",dst);*/
	imwrite("result.jpg",dst);
	
	Mat In=Mat::zeros(Size(W/2,H/2),CV_32F);
	Mat D1=Mat::zeros(Size(W/2,H/2),CV_32F);
	Mat D2=Mat::zeros(Size(W/2,H/2),CV_32F);
	Mat D3=Mat::zeros(Size(W/2,H/2),CV_32F);

	for(int h=0;h<In.rows;h++)
	{
		for (int w=0;w<In.cols;w++)
		{
			In.at<float>(h,w)=dst.at<float>(h,w);
			D1.at<float>(h,w)=dst.at<float>(h,w+W/2);
			D2.at<float>(h,w)=dst.at<float>(h+H/2,w);
			D3.at<float>(h,w)=dst.at<float>(h+H/2,w+W/2);

		}
	}

#if 1
	imwrite("top_left.jpg",In);
	imwrite("top_right.jpg",D1);
	imwrite("bottom_left.jpg",D2);
	imwrite("bottom_right.jpg",D3);
#endif

	//��cv32fת��Ϊcv8u
	Mat ID1,ID2,ID3;
	double min=0;
	double max=0;
	minMaxLoc(D1,&min,&max);
	D1.convertTo(ID1,CV_8UC1,255.0/(max-min),-255.0/min);
	minMaxLoc(D2,&min,&max);
	D2.convertTo(ID2,CV_8UC1,255.0/(max-min),-255.0/min);
	minMaxLoc(D3,&min,&max);
	D3.convertTo(ID3,CV_8UC1,255.0/(max-min),-255.0/min);

	Mat* mats=new Mat[3];
	mats[0]=ID1;
	mats[1]=ID2;
	mats[2]=ID3;
	//����ֱ��ͼ
	//���ҶȻ���Ϊ256��
	int gray_bins=256;
	int histSize[]={gray_bins};

	//�Ҷ�ֵ��0-255
	float grayRanges[]={0,255};
	const float* ranges[]={grayRanges};
	MatND hist;

	int channels[]={0};

	//����С���任��ֱ��ͼ
	calcHist(mats,1,channels,Mat(),hist,1,histSize,ranges,true,false);

#if 1
	double maxValue=0;
	minMaxLoc(hist,0,&maxValue,0,0);

	//����ֱ��ͼ
	Mat histImg=Mat::zeros(256,256,CV_8UC1);
	for (int i=0;i<gray_bins;i++)
	{
		float binValue=hist.at<float>(i,0);
		cout<<"binValue: "<<binValue<<endl;
		int intensity=cvRound(binValue*255/maxValue);
		rectangle(histImg,Point(i,256),Point(i+1,256-intensity),Scalar(255),1,8);

	}

	namedWindow("Histogram",1);
	imshow("Histogram",histImg);
	//waitKey(0);
#endif

	//����Tc
	double area=0;
	double areaAll=0;
	//���
	for (int i=0;i<gray_bins;i++)
	{
		float binValue=hist.at<float>(i,0);
		areaAll+=binValue;

	}
	cout<<"areaAll: "<<areaAll<<endl;

	for (int i=0;i<gray_bins;i++)
	{
		hist.at<float>(i,0)=hist.at<float>(i,0)/areaAll;
	}

	int Tc=0;
	for (int i=0;i<gray_bins;i++)
	{
		float binValue=hist.at<float>(i,0);
		area+=binValue;
		if (area>=0.85)
		{
			Tc=i;
			break;
		}
	}

	//��TcС��30ʱ��Tcȡ30
	Tc=Tc>30?Tc:30;

	cout<<"Tc:  "<<Tc<<endl;
	//����En
	//mask
	Mat mask=Mat::zeros(D1.size(),CV_8UC1);
	for (int h=0;h<D1.rows;h++)
	{
		for (int w=0;w<D1.cols;w++)
		{
			float _d1=ID1.at<uchar>(h,w);
			float _d2=ID2.at<uchar>(h,w);
			float _d3=ID3.at<uchar>(h,w);
			double En=sqrt(_d1*_d1+_d2*_d2+_d3*_d3);
			//cout<<"EN: "<<En<<endl;
			if(En>Tc)
				mask.at<uchar>(h,w)=255;
			
		}
	}
	//bitwise_not(mask,mask);
#if 1
	namedWindow("mask",1);
	imshow("mask",mask);
	waitKey();
#endif
	delete[] mats;
	return 1;

}