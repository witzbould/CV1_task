#include "Utils.h"

#include <list>
#include <opencv2/imgproc/imgproc.hpp>


Utils::Utils()
{
}


Utils::~Utils()
{
}

cv::Mat Utils::convertImgToGray(const cv::Mat & Img)
{
  cv::Mat ret = cv::Mat(Img);
  cvtColor(ret, ret, cv::COLOR_BGR2GRAY);

  return ret;
}

cv::Mat Utils::convertImgToBGR(const cv::Mat & Img)
{
  cv::Mat ret = cv::Mat(Img);
  cvtColor(ret, ret, cv::COLOR_GRAY2BGR);

  return ret;
}

bool Utils::isPointOffMat(const cv::Mat & Img, const cv::Point & Pnt)
{
  return (Pnt.x < 0 || Pnt.y < 0 || Pnt.x < Img.cols - 1 || Pnt.y > Img.rows - 1);
}

bool Utils::isPointWithinMat(const cv::Mat & Img, const cv::Point & Pnt)
{
  return !isPointOffMat(Img, Pnt);
}

void Utils::drawPointInMat(cv::Mat & Img, const cv::Point & Pnt, const cv::Vec3b & Color = cv::Vec3b(0, 0, 0))
{
  Img.at<cv::Vec3b>(Pnt) = Color;
}

void Utils::drawPointInMat(cv::Mat & Img, const cv::Point & Pnt, uchar Color)
{
  Img.at<cv::Vec3b>(Pnt) = cv::Vec3b(Color, Color, Color);
}

cv::Mat Utils::convolveMatWithSobel(const cv::Mat & Img)
{
  // Accept only char type matrices
  CV_Assert(Img.depth() == CV_8U);

  cv::Mat Res;
  int row_count = Img.rows - 1;
  int col_count = (Img.cols - 1) * Img.channels();
  int i,
    j,
    val_col_left,
    val_col_right,
    val_result;
  const uchar *row_cur,
    *row_prev,
    *row_next;
  uchar *row_res;
  std::list<int> cols_calculated;

  // Create result image with same size and type like the origninal one. Initialize pixel values with 0.
  Res.create(Img.size(), Img.type());
  Res = cv::Scalar::all(0);

  for (i = 1; i < row_count; ++i) {
    // Get pointer to rows
    row_prev = Img.ptr<uchar>(i - 1);
    row_cur = Img.ptr<uchar>(i);
    row_res = Res.ptr<uchar>(i);
    row_next = Img.ptr<uchar>(i + 1);
    // Clear previous calculated values per row
    cols_calculated.clear();

    for (j = 1; j < col_count; ++j) {
      // Calculate value of the left operator column if there are no previously created ones.
      // Else get the value and clear it from list.
      if (cols_calculated.size() < 2) {
        val_col_left = row_prev[j - 1] + ((uint)row_cur[j - 1] << 1) + row_next[j - 1];
      }
      else {
        val_col_left = cols_calculated.front();
        cols_calculated.pop_front();
      }

      // Always calculate the right column; [j +- 1] are the adjacent pixels
      val_col_right = row_prev[j + 1] + ((uint)row_cur[j + 1] << 1) + row_next[j + 1];
      // Calculate the result value
      val_result = val_col_right - val_col_left;

      // Save the right column value to list
      cols_calculated.push_back(val_col_right);
      // Write result value to the result image
      row_res[j] = cv::saturate_cast<uchar>(val_result);
    }
  }

  return Res;
}

cv::Mat Utils::convolveMatWithExpMask1D(const cv::Mat & Img)
{
  // Accept only char type matrices
  CV_Assert(Img.depth() == CV_8U);

  cv::Mat Res;
  double tau = 0.75;

  Res.create(Img.size(), Img.type());
  Res = cv::Scalar::all(0);

  for (int i = 1; i < Img.total(); i++) {
    // Calculate new value for every pixel by using the value of the previous pixel
    double value = tau * Res.at<uchar>(i - 1) + tau* Img.at<uchar>(i);

    // Write result value to the result image
    Res.at<uchar>(i) = cv::saturate_cast<uchar>(value);
  }

  return Res;
}