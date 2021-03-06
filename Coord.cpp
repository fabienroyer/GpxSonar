// Description:  Coordinate calculations.

#include "Coord.h"

#define PI                 3.14159265358979
#define EPSILON            5.e-14

// Ellipsoid is initialized to WGS84 ellipsoid.
const CEllipsoid CLatLon::m_Ellipsoid = CEllipsoid(6378137.00, 298.257223563);
// Radius of the Earth in meters.
const double CLatLon::m_Radius = 6366707.01896486;
// Degrees to radians conversion.
const double CLatLon::m_Deg2Rad = 1.74532925199433E-02;
// UTM Zone letters.
const std::string CLatLon::m_strZoneLetters = "CDEFGHJKLMNPQRSTUVWX";

// -------------------------------------------------------------------------
// METHOD:  CLatLon::SphericalDistance()
/*! 
   \brief  Computes great-circle distance from this point to point P.

   \author fizzymagic
   \date   9/6/2003

   \return  [double] - Distance between this point and P in meters.

   \param P [CLatLon&] - Point to which to compute distance.

*/
// -------------------------------------------------------------------------
double CLatLon::SphericalDistance(CLatLon& P)
{
   double dDeltaLong = (m_Longitude - P.m_Longitude) * m_Deg2Rad;
   double dLat1 = m_Latitude * m_Deg2Rad;
   double dLat2 = P.m_Latitude * m_Deg2Rad;
   double dDeltaLat = dLat1 - dLat2;
   double dAngle = acos(sin(dLat1) * sin(dLat2) 
                     + cos(dLat1) * cos(dLat2) * cos(dDeltaLong));
   double dDistance = m_Radius * dAngle;

   // The above formula doesn't work well for small distances.
   // So if the distance is small, use simple linear approximation.
   if (dDistance < .01) {
      dDeltaLong *= cos(dLat2);
      dDistance = m_Radius * sqrt(dDeltaLat*dDeltaLat + dDeltaLong*dDeltaLong);
   }
   return dDistance;
}

// -------------------------------------------------------------------------
// METHOD:  CLatLon::SphericalProjection()
/*! 
   \brief  Computes a projection along a great circle.

   \author fizzymagic
   \date   9/6/2003

   \return  [CLatLon] - Projected coordinates.

   \param dAzimuth [double] - Forward azimuth in degrees.
   \param dDistance [double] - Distance in meters.
*/
// -------------------------------------------------------------------------
CLatLon CLatLon::SphericalProjection(double dAzimuth, double dDistance)
{
   CLatLon RetCoord;
   dAzimuth *= m_Deg2Rad;

   double dLat1 = m_Deg2Rad * m_Latitude;
   double dLong1 = m_Deg2Rad * m_Longitude;
   double s = dDistance / m_Radius;

   double dLat2 = asin(sin(dLat1)*cos(s) + cos(dLat1)*sin(s)*cos(dAzimuth));
   double dLong2 = atan2(sin(dAzimuth)*sin(s)*cos(dLat1), cos(s) - sin(dLat1)*sin(dLat2));
   RetCoord.m_Latitude = dLat2 / m_Deg2Rad;
   RetCoord.m_Longitude = fmod(dLong2 / m_Deg2Rad + 180., 360.) - 180.;

   return RetCoord;
}

// -------------------------------------------------------------------------
// METHOD:  CLatLon::SphericalDistance()
/*! 
   \brief  Computes great-circle distance from this point to the great circle
           connecting P1 and P2.

   \author fizzymagic
   \date   9/6/2003

   \return  [double] - Distance between this point and the GC in meters.
                       If this point is not between the other points, the
                       returned distance will be negative.

   \param P1 [CLatLon&] - First point defining great circle.
   \param P2 [CLatLon&] - Second point defining great circle.
*/
// -------------------------------------------------------------------------
double CLatLon::SphericalDistance(CLatLon& P1, CLatLon& P2)
{
   double dSign = 1.;
   if (!IsBetween(P1, P2)) {
      dSign = -1.;
   }
   CCartesianCoord P = ToSphericalCartesian();
   CCartesianCoord N = P1.SphericalCross(P2);
   N.Normalize();
   return dSign * fabs(m_Radius * (PI/2. - acos(N.dot(P))));
}

// -------------------------------------------------------------------------
// METHOD:  CLatLon::IsBetween()
/*! 
   \brief  Approximate test to check if this point is between P1 and P2.

   \author fizzymagic
   \date   9/6/2003

   \return  [bool] - true if point it between others; false otherwise.

   \param P1 [CLatLon&] - First point.
   \param P2 [CLatLon&] - Second point.
*/
// -------------------------------------------------------------------------
bool CLatLon::IsBetween(CLatLon& P1, CLatLon& P2)
{
   // Approximate scale factor for longitudes.
   double cosLat = cos(m_Latitude * m_Deg2Rad);
   double u = (m_Longitude - P1.m_Longitude) * (P2.m_Longitude - P1.m_Longitude) * cosLat * cosLat
      + (m_Latitude - P1.m_Latitude) * (P2.m_Latitude - P1.m_Latitude);
   u /= (P2.m_Longitude - P1.m_Longitude) * (P2.m_Longitude - P1.m_Longitude) * cosLat * cosLat
      + (P2.m_Latitude - P1.m_Latitude) * (P2.m_Latitude - P1.m_Latitude);
   
   // u is the relative position on the line connecting P1 and P2.
   return (u >= 0. && u < 1.);
}

// -------------------------------------------------------------------------
// METHOD:  CLatLon::ParseCoords()
/*! 
   \brief  Parses single coordinate string in a variety of formats.

   \author fizzymagic
   \date   9/6/2003

   \return  [bool] - true if coordinates were parsed without error.

   \param szCoordString [const char *] - Coordinate string.
*/
// -------------------------------------------------------------------------
/*
bool CLatLon::ParseCoords(const char *szCoordString)
{
   if (ParseUTM(szCoordString)) {
      return true;
   }

   std::string strCoordString(szCoordString);
   CleanCoordString(strCoordString);
   std::string::size_type iSplitPoint;
   std::string strLatString, strLongString;
   if ((iSplitPoint = strCoordString.find_first_of(",EW")) == std::string::npos) {
      return false;
   }
   strLatString.assign(strCoordString, 0, iSplitPoint);
   strLongString.assign(strCoordString, iSplitPoint, strCoordString.size() - iSplitPoint);
   if (strLongString[0] == ',') strLongString[0] = ' ';

   return ParseCoords(strLatString.c_str(), strLongString.c_str());
}
*/
// -------------------------------------------------------------------------
// METHOD:  CLatLon::ParseCoords()
/*! 
   \brief  Parses separate lat/long coordinate strings in a variety of formats.

   \author fizzymagic
   \date   9/6/2003

   \return  [bool] - true if coordinates were parsed without error.

   \param szLatString [const char *] - Coordinate string for latitude.
   \param szLongString [const char *] - Coordinate string for longitude.
*/
// -------------------------------------------------------------------------
/*
bool CLatLon::ParseCoords(const char *szLatString, const char *szLongString)
{
   std::string strLatString(szLatString), strLongString(szLongString);
   CleanCoordString(strLatString);
   CleanCoordString(strLongString);

   int iLatSign = 1;
   std::string::iterator it = strLatString.begin();
   std::string::iterator eit = strLatString.end();
   std::string strBuffer;
   double dLatitude, dLongitude;

   if (*it == 'N') {
      it++;
      while (::isspace(*it) && it != eit) it++;
   }
   else if (*it == 'S') {
      iLatSign = -1;
      it++;
      while (::isspace(*it) && it != eit) it++;
   }

   strBuffer.erase();
   strBuffer.append(it, eit);
   if (ParseDegreeString(strBuffer, &dLatitude)) {
      dLatitude *= iLatSign;
   }
   else return false;

   int iLongSign = 1;
   it = strLongString.begin();
   eit = strLongString.end();
   if (*it == 'E') {
      it++;
      while (::isspace(*it) && it != eit) it++;
   }
   else if (*it == 'W') {
      iLongSign = -1;
      it++;
      while (::isspace(*it) && it != eit) it++;
   }

   strBuffer.erase();
   strBuffer.append(it, eit);
   if (ParseDegreeString(strBuffer, &dLongitude)) {
      dLongitude *= iLongSign;
   }
   else return false;

   m_Latitude = dLatitude;
   m_Longitude = dLongitude;

   return true;
}
*/
// -------------------------------------------------------------------------
// METHOD:  CLatLon::ParseDegreeString()
/*! 
   \brief  Parses a string containing degrees in various formats.

   \author fizzymagic
   \date   9/8/2003

   \return  [bool] - true if successful parse, false otherwise.

   \param strDegrees [std::string&] - Degree string to parse.
   \param dResult [double *] - Result of parsed string, in degrees.
*/
// -------------------------------------------------------------------------
/*
bool CLatLon::ParseDegreeString(std::string& strDegrees, double *dResult)
{
   std::string::iterator it = strDegrees.begin();
   std::string::iterator eit = strDegrees.end();
   double dTemp, dDegrees = 0.;
   int iSign = 1;
   std::string strBuffer;

   CleanCoordString(strDegrees);

   if (it == eit || !(::isdigit(*it) || *it == '-')) {
      *dResult = 0.;
      return false;
   }
   if (*it == '-') {
      iSign = -1;
      it++;
   }

   strBuffer.erase();
   while (::isdigit(*it) || *it == '.' && it != eit) {
      if (it == eit) return false;
      strBuffer.append(1, *it);
      it++;
   }
   dTemp = atof(strBuffer.c_str());
   if (dTemp < -180. || dTemp > 360.) {
      return false;
   }
   else {
      dDegrees = dTemp;
   }
   while (::isspace(*it) && it != eit) it++;
   if (it != eit) {
      if (!::isdigit(*it)) return false;
      strBuffer.erase();
      while (::isdigit(*it) || *it == '.' && it != eit) {
         strBuffer.append(1, *it);
         it++;
      }
      dTemp = atof(strBuffer.c_str());
      if (dTemp < 0. || dTemp >= 60.) {
         return false;
      }
      else {
         dDegrees += dTemp / 60.;
      }
      while (::isspace(*it) && it != eit) it++;
      if (it != eit) {
         if (!::isdigit(*it)) return false;
         strBuffer.erase();
         while (::isdigit(*it) || *it == '.' && it != eit) {
            strBuffer.append(1, *it);
            it++;
         }
         dTemp = atof(strBuffer.c_str());
         if (dTemp < 0. || dTemp >= 60.) {
            return false;
         }
         else {
            dDegrees += dTemp / 3600.;
         }
      }
   }
   *dResult = dDegrees * iSign;
   return true;
}
*/
// -------------------------------------------------------------------------
// METHOD:  CLatLon::ParseUTM()
/*! 
   \brief  Parses UTM coordinate strings.

   \author fizzymagic
   \date   9/6/2003

   \return  [bool] - true if the coordinate string is UTM and as parsed
                     without error.

   \param szCoordString [const char *] - UTM coordinate string.
*/
// -------------------------------------------------------------------------
/*
bool CLatLon::ParseUTM(const char *szCoordString)
{
   std::string strCoordString(szCoordString);
   std::transform(strCoordString.begin(), strCoordString.end(), strCoordString.begin(), ::toupper);
   CleanCoordString(strCoordString);

   std::string::iterator it = strCoordString.begin();
   std::string::iterator eit = strCoordString.end();
   std::string strBuffer;

   int iZone, iZoneLetter;
   double dEasting, dNorthing;

   if (!::isdigit(*it)) return false;
   strBuffer.erase();
   while (::isdigit(*it) && it != eit) {
      strBuffer.append(1, *it);
      it++;
   }
   iZone = atoi(strBuffer.c_str());
   if (it == eit) return false;
   if ((iZoneLetter = m_strZoneLetters.find(*(it++))) == std::string::npos) return false;

   strBuffer.erase();
   while (!::isdigit(*it) && it != eit) it++;
   if (it == eit) return false;
   while (::isdigit(*it) && it != eit) {
      strBuffer.append(1, *it);
      it++;
   }
   if (it == eit) return false;
   dEasting = atof(strBuffer.c_str());

   strBuffer.erase();
   while (!::isdigit(*it) && it != eit) it++;
   if (it == eit) return false;
   while (::isdigit(*it) && it != eit) {
      strBuffer.append(1, *it);
      it++;
   }
   dNorthing = atof(strBuffer.c_str());

   return ConvertUTM(iZone, m_strZoneLetters[iZoneLetter], dEasting, dNorthing);
}
*/
// -------------------------------------------------------------------------
// METHOD:  CLatLon::LatToDDD()
/*! 
   \brief  Converts latitude coordinate into DD.DDDDDD format.

   \author fizzymagic7
   \date   9/6/2003

   \return  [std::string] - Latitude in DD.DDDDDD format.
*/
// -------------------------------------------------------------------------
/*
std::string CLatLon::LatToDDD(void)
{
   std::ostringstream Stream;
   Stream << std::fixed << std::setprecision(6) 
          << m_Latitude;
   return Stream.str();
}
*/
// -------------------------------------------------------------------------
// METHOD:  CLatLon::LatToDMM()
/*! 
   \brief  Converts latitude coordinate into DD MM.MMM format.

   \author fizzymagic7
   \date   9/6/2003

   \return  [std::string] - Latitude in DD MM.MMM format.
*/
// -------------------------------------------------------------------------
/*
std::string CLatLon::LatToDMM(void)
{
   std::ostringstream Stream;
   int iLatDegrees = (int) floor(fabs(m_Latitude));
   double dLatMinutes = 60. * (fabs(m_Latitude) - floor(fabs(m_Latitude)));
   Stream << std::fixed << std::setprecision(3)
          << ((m_Latitude >= 0.)?'N':'S') << " "
          << iLatDegrees  << " "
          << std::setfill('0') << std::setw(6) << dLatMinutes;
   return Stream.str();
}
*/
// -------------------------------------------------------------------------
// METHOD:  CLatLon::LatToDMS()
/*! 
   \brief  Converts latitude coordinate into DD MM SS.SS format.

   \author fizzymagic7
   \date   9/6/2003

   \return  [std::string] - Latitude in DD MM SS.SS format.
*/
// -------------------------------------------------------------------------
/*
std::string CLatLon::LatToDMS(void)
{
   std::ostringstream Stream;
   int iLatDegrees = (int) floor(fabs(m_Latitude));
   int iLatMinutes = (int) floor(60. * (fabs(m_Latitude) - floor(fabs(m_Latitude))));
   double dLatSeconds = 60. * (fabs(m_Latitude * 60.) - floor(fabs(m_Latitude * 60.)));
   Stream << std::fixed << std::setprecision(2)
          << ((m_Latitude >= 0.)?'N':'S') << " "
          << iLatDegrees  << " "
          << std::setfill('0') << std::setw(2) << iLatMinutes  << " "
          << std::setfill('0') << std::setw(5) << dLatSeconds;
   return Stream.str();
}
*/
// -------------------------------------------------------------------------
// METHOD:  CLatLon::LongToDDD()
/*! 
   \brief  Converts longitude coordinate into DDD.DDDDDD format.

   \author fizzymagic7
   \date   9/6/2003

   \return  [std::string] - Longitude in DDD.DDDDDD format.
*/
// -------------------------------------------------------------------------
/*
std::string CLatLon::LongToDDD(void)
{
   std::ostringstream Stream;
   Stream << std::fixed << std::setprecision(6) 
          << m_Longitude;
   return Stream.str();
}
*/
// -------------------------------------------------------------------------
// METHOD:  CLatLon::LongToDMM()
/*! 
   \brief  Converts longitude coordinate into DDD MM.MMM format.

   \author fizzymagic7
   \date   9/6/2003

   \return  [std::string] - Longitude in DDD MM.MMM format.
*/
// -------------------------------------------------------------------------
/*
std::string CLatLon::LongToDMM(void)
{
   std::ostringstream Stream;
   int iLongDegrees = (int) floor(fabs(m_Longitude));
   double dLongMinutes = 60. * (fabs(m_Longitude) - floor(fabs(m_Longitude)));
   Stream << std::fixed << std::setprecision(3)
          << ((m_Longitude >= 0.)?'E':'W') << " "
          << iLongDegrees  << " "
          << std::setfill('0') << std::setw(6) << dLongMinutes;
   return Stream.str();
}
*/
// -------------------------------------------------------------------------
// METHOD:  CLatLon::LongToDMS()
/*! 
   \brief  Converts longitude coordinate into DDD MM SS.SS format.

   \author fizzymagic7
   \date   9/6/2003

   \return  [std::string] - Longitude in DDD MM SS.SS format.
*/
// -------------------------------------------------------------------------
/*
std::string CLatLon::LongToDMS(void)
{
   std::ostringstream Stream;
   int iLongDegrees = (int) floor(fabs(m_Longitude));
   int iLongMinutes = (int) floor(60. * (fabs(m_Longitude) - floor(fabs(m_Longitude))));
   double dLongSeconds = 60. * (fabs(m_Longitude * 60.) - floor(fabs(m_Longitude * 60.)));
   Stream << std::fixed << std::setprecision(2)
          << ((m_Longitude >= 0.)?'E':'W') << " "
          << iLongDegrees  << " "
          << std::setfill('0') << std::setw(2) << iLongMinutes  << " "
          << std::setfill('0') << std::setw(5) << dLongSeconds;
   return Stream.str();
}
*/
// -------------------------------------------------------------------------
// METHOD:  CLatLon::ToDDD()
/*! 
   \brief  Converts coordinates into DDD.DDDDDD format.

   \author fizzymagic
   \date   9/6/2003

   \return  [std::string] - Output string containing both coordinates in
                            DDD.DDDDDD format.

   This method converts both latitude and longitude into a single string
   in which both are formatted as decimal degrees.  The string will look
   like:

   DD.DDDDDD,-DDD.DDDDDD

   with the proper signs in front of both parts. No space is placed in 
   between the coordinates in order to facilitate importing into 
   spreadsheets.
*/
// -------------------------------------------------------------------------
/*
std::string CLatLon::ToDDD(void)
{
   std::ostringstream Stream;
   Stream << LatToDDD() << "," << LongToDDD();
   return Stream.str();
}
*/
// -------------------------------------------------------------------------
// METHOD:  CLatLon::ToDMM()
/*! 
   \brief  Converts coordinates into DDD MM.MMM format.

   \author fizzymagic
   \date   9/6/2003

   \return  [std::string] - Output string containing both coordinates in
                            DDD MM.MMM format.

   This method converts both latitude and longitude into a single string
   in which both are formatted as degrees and decimal minutes.  The string 
   will look like:

   N DD MM.MMM, W DDD MM.MMM
*/
// -------------------------------------------------------------------------
/*
std::string CLatLon::ToDMM(void)
{
   std::ostringstream Stream;
   Stream << LatToDMM() << ", " << LongToDMM();
   return Stream.str();
}
*/
// -------------------------------------------------------------------------
// METHOD:  CLatLon::ToDMS()
/*! 
   \brief  Converts coordinates into DDD MM SS.SS format.

   \author fizzymagic
   \date   9/6/2003

   \return  [std::string] - Output string containing both coordinates in
                            DDD MM SS.SS format.

   This method converts both latitude and longitude into a single string
   in which both are formatted as degrees, minutes and seconds.  The string 
   will look like:

   N DD MM SS.SS, W DDD MM SS.SS
*/
// -------------------------------------------------------------------------
/*
std::string CLatLon::ToDMS(void)
{
   std::ostringstream Stream;
   Stream << LatToDMS() << ", " << LongToDMS();
   return Stream.str();
}
*/
// -------------------------------------------------------------------------
// METHOD:  CLatLon::ToUTM()
/*! 
   \brief  Converts coordinates into UTM coordinates.

   \author fizzymagic
   \date   9/6/2003

   \return  [std::string] - Output string in which the coordinates are
                            in standard UTM format.

   This method converts the coordinates into UTM coordinates using the
   WGS84 ellipsoid.  The output string will look like:

   ZZL E eeeeee N nnnnnnn 

   where ZZ is the zone number, L is the zone letter, eeeeee is the easting,
   and nnnnnnn is the northing.
*/
// -------------------------------------------------------------------------
/*
std::string CLatLon::ToUTM(void)
{
   std::ostringstream Stream;
   int iZone = GetZone();
   char cZoneLetter = GetZoneLetter();
   const double k0 = 0.9996;
   const double dEastingOffset = 5.e5;
   const double dNorthingOffsetSouth = 1.e7;

   double a = m_Ellipsoid.m_a;
   double flat = 1./m_Ellipsoid.m_fInv;
   double ecc2 = 2. * flat - flat * flat;
   double ecc12 = (ecc2)/(1. - ecc2);


   double dLatitude = m_Latitude * m_Deg2Rad;
   double dLongitude = (fmod(m_Longitude + 180., 360.) - 180.) * m_Deg2Rad;
   double dCentralLongitude = ((iZone - 1) * 6. - 180. + 3.) * m_Deg2Rad;

   double N, T, C, A, M;
   double dUTMNorthing, dUTMEasting;

   N = a / sqrt(1. - ecc2*sin(dLatitude)*sin(dLatitude));
   T = tan(dLatitude)*tan(dLatitude);
   C = ecc12*cos(dLatitude)*cos(dLatitude);
   A = cos(dLatitude)*(dLongitude - dCentralLongitude);

   M = a * ((1. - ecc2/4. - 3.*ecc2*ecc2/64. - 5.*ecc2*ecc2*ecc2/256.)*dLatitude 
         - (3.*ecc2/8. + 3.*ecc2*ecc2/32. + 45.*ecc2*ecc2*ecc2/1024.)*sin(2.*dLatitude)
         + (15.*ecc2*ecc2/256. + 45.*ecc2*ecc2*ecc2/1024.)*sin(4.*dLatitude) 
         - (35.*ecc2*ecc2*ecc2/3072.)*sin(6.*dLatitude));
   
   dUTMEasting = (double)(k0*N*(A+(1. - T + C)*A*A*A/6.
               + (5. - 18.*T+T*T + 72.*C - 58.*ecc12)*A*A*A*A*A/120.)
               + dEastingOffset);

   dUTMNorthing = (double)(k0*(M + N*tan(dLatitude)*(A*A/2. + (5. - T + 9.*C + 4.*C*C)*A*A*A*A/24.
             + (61. - 58.*T+T*T + 600.*C - 330.*ecc12)*A*A*A*A*A*A/720.)));

   if (m_Latitude < 0.) {
      dUTMNorthing += dNorthingOffsetSouth;
   }

   dUTMEasting = floor(dUTMEasting + 0.5);
   dUTMNorthing = floor(dUTMNorthing + 0.5);

   Stream << std::fixed << std::setprecision(0)
      << iZone << cZoneLetter << " "
      << "E " << dUTMEasting << " " 
      << "N " << dUTMNorthing;
   return Stream.str();
}
*/
// -------------------------------------------------------------------------
// METHOD:  CLatLon::VincentyDistance()
/*! 
   \brief  Calculates the distance between this point and P using the Vincenty
           method.

   \author fizzymagic
   \date   9/6/2003

   \return  [double] - Distance between this point and P in meters.

   \param P [CLatLon&] - Point to which to compute distance.

   This method computes a high-accuracy distance between this point and P
   using the Vincenty method and the WGS84 ellipsoid.
*/
// -------------------------------------------------------------------------
double CLatLon::VincentyDistance(CLatLon& P)
{
   double Tmp1, Tmp2;
   return this->VincentyDistance(P, &Tmp1, &Tmp2);
}

// -------------------------------------------------------------------------
// METHOD:  CLatLon::VincentyDistance()
/*! 
   \brief  Calculates the distance and forward and reverse azimuths between 
           this point and P using the Vincenty method.

   \author fizzymagic
   \date   9/6/2003

   \return  [double] - Distance between this point and P in meters.

   \param P [CLatLon&] - Point to which to compute distance.
   \param pForwardAzimuth [double *] - Pointer to parameter to receive 
                                       forward azimuth in degrees.
   \param pReverseAzimuth [double *] - Pointer to parameter to receive
                                       reverse azimuth in degrees.


   This method computes a high-accuracy distance between this point and P
   using the Vincenty method and the WGS84 ellipsoid.  It also calculates 
   and returns the forward and reverse azimuths.
*/
// -------------------------------------------------------------------------
double CLatLon::VincentyDistance(CLatLon& P, double *pForwardAzimuth, double *pReverseAzimuth)
{
   if (m_Latitude == P.m_Latitude
      && m_Longitude == P.m_Longitude) {
      *pForwardAzimuth = 0.;
      *pReverseAzimuth = 0.;
      return 0.;
   }

   double dLat1 = m_Deg2Rad * m_Latitude;
   double dLat2 = m_Deg2Rad * P.m_Latitude;
   double dLong1 = m_Deg2Rad * m_Longitude;
   double dLong2 = m_Deg2Rad * P.m_Longitude;

   double a0 = m_Ellipsoid.m_a;
   double flat = 1./m_Ellipsoid.m_fInv;
   double r = 1. - flat;
   double b0 = a0 * r;

   double tanu1 = r * tan(dLat1);
   double tanu2 = r * tan(dLat2);

   double dtmp;
   dtmp = atan(tanu1);
   double cosu1 = cos(dtmp);
   double sinu1 = sin(dtmp);

   dtmp = atan(tanu2);
   double cosu2 = cos(dtmp);
   double sinu2 = sin(dtmp);

   double omega = dLong2 - dLong1;
   double lambda = omega;

   double testlambda, ss1, ss2, ss, cs, tansigma, sinalpha, cosalpha, cosalpha2, c2sm, c, dDeltaLambda;

   do {
      testlambda = lambda;
      ss1 = cosu2 * sin(lambda);
      ss2 = cosu1 * sinu2 - sinu1 * cosu2 * cos(lambda);
      ss = sqrt(ss1*ss1 + ss2 * ss2);
      cs = sinu1 * sinu2 + cosu1 * cosu2 * cos(lambda);
      tansigma = ss / cs;
      sinalpha = cosu1 * cosu2 * sin(lambda) / ss;
      dtmp = asin(sinalpha);
      cosalpha = cos(dtmp);
      cosalpha2 = cosalpha * cosalpha; 
      c2sm = cs - 2.*sinu1*sinu2/cosalpha2;
      c = flat/16. * cosalpha2*(4. + flat*(4. - 3.*cosalpha2));
      lambda = omega + (1. - c)*flat*sinalpha*(asin(ss) + c*ss*(c2sm + c*cs*(-1. + 2.*c2sm*c2sm)));
      dDeltaLambda = fabs(testlambda - lambda);
   } while (dDeltaLambda > EPSILON);

   double u2 = cosalpha2 * (a0*a0 - b0*b0)/(b0*b0);
   double a = 1. + (u2 / 16384.) * (4096. + u2 * (-768. + u2 * (320. - 175. * u2)));
   double b = (u2 / 1024.) * (256. + u2 * (-128. + u2 * (74. - 47. * u2)));

   double dsigma = b * ss * (c2sm + (b / 4.) * (cs * (-1. + 2. * c2sm*c2sm) 
                 - (b / 6.) * c2sm * (-3. + 4. * ss*ss) * (-3. + 4. * c2sm*c2sm)));

   double s = b0 * a * (asin(ss) - dsigma);

   double alpha12 = atan2(cosu2 * sin(lambda), (cosu1 * sinu2 - sinu1 * cosu2 * cos(lambda)))/m_Deg2Rad;
   double alpha21 = atan2(cosu1 * sin(lambda), (-sinu1 * cosu2 + cosu1 * sinu2 * cos(lambda)))/m_Deg2Rad;

   *pForwardAzimuth = fmod(alpha12 + 360., 360.);
   *pReverseAzimuth = fmod(alpha21 + 180., 360.);

   return s;
}

// -------------------------------------------------------------------------
// METHOD:  CLatLon::VincentyProjection()
/*! 
   \brief  Computes a projection along a geodesic using the Vincenty method.

   \author fizzymagic
   \date   9/6/2003

   \return  [CLatLon] - Projected coordinates.

   \param dAzimuth [double] - Forward azimuth in degrees.
   \param dDistance [double] - Distance in meters.
*/
// -------------------------------------------------------------------------
CLatLon CLatLon::VincentyProjection(double dAzimuth, double dDistance)
{
   CLatLon RetCoord;

   double lastsigma, twosigmam, ss, cs, c2sm, deltasigma;
   double term1, term2, term3, term4;

   dAzimuth *= m_Deg2Rad;
   double dLat1 = m_Deg2Rad * m_Latitude;
   double dLong1 = m_Deg2Rad * m_Longitude;
   double s = dDistance;

   double a0 = m_Ellipsoid.m_a;
   double flat = 1./m_Ellipsoid.m_fInv;
   double r = 1. - flat;
   double b0 = a0 * r;
   double tanu1 = r * tan(dLat1);

   double tansigma1 = tanu1 / cos(dAzimuth);
   double u1 = atan(tanu1);
   double sinu1 = sin(u1);
   double cosu1 = cos(u1);

   double sinalpha = cosu1 * sin(dAzimuth);
   double cosalpha = sqrt(1. - sinalpha*sinalpha);

   double usqr = cosalpha*cosalpha * (a0*a0 - b0*b0) / (b0*b0);

   term1 = usqr / 16384.;
   term2 = 4096. + usqr * (-768. + usqr * (320. - 175. * usqr));
   double a = 1. + term1 * term2;
   double b = usqr / 1024. * (256. + usqr * (-128. + usqr * (74. - 47. * usqr)));
                                       
   double sigma = s / (b0 * a);
   double sigma1 = atan(tansigma1);

   do {
      lastsigma = sigma;
      twosigmam = 2.* sigma1 + sigma;
      ss = sin(sigma);
      cs = cos(sigma);
      c2sm = cos(twosigmam);

      deltasigma = b * ss * (c2sm + b / 4. * (cs * (-1. + 2. * c2sm*c2sm) 
                 - b / 6. * c2sm * (-3. + 4. * ss*ss) * (-3. + 4. * c2sm*c2sm)));

      sigma = s / (b0 * a) + deltasigma;

   } while (fabs(sigma - lastsigma) > EPSILON);

   twosigmam = 2. * sigma1 + sigma;
   ss = sin(sigma);
   cs = cos(sigma);
   c2sm = cos(twosigmam);
   term1 = sinu1 * cs + cosu1 * ss * cos(dAzimuth);
   term4 = sinu1 * ss - cosu1 * cs * cos(dAzimuth);
   term2 = sinalpha*sinalpha + term4*term4;
   term3 = r * sqrt(term2);

   double dLat2 = atan2(term1, term3);

   term1 = ss * sin(dAzimuth);
   term2 = cosu1 * cs - sinu1 * ss * cos(dAzimuth);
   double tanlambda = term1 / term2;
   double lambda = atan2(term1, term2);

   double c = flat / 16. * cosalpha*cosalpha * (4. + flat * (4. - 3. * cosalpha*cosalpha));

   double omega = lambda - (1. - c) * flat * sinalpha * (sigma + c * ss * (c2sm + c * cs * (-1. + 2. * c2sm*c2sm)));

   double dLong2 = dLong1 + omega;

   term1 = -sinu1 * ss + cosu1 * cs * cos(dAzimuth);

   RetCoord.m_Latitude = fmod(dLat2/m_Deg2Rad, 360.);
   RetCoord.m_Longitude = fmod(dLong2/m_Deg2Rad, 360.);

//   double az21 = atan2(sinalpha, term1);
//   az21 = fmod(az21/m_Deg2Rad + 180., 360.);

   return RetCoord;
}

// -------------------------------------------------------------------------
// METHOD:  CLatLon::CleanCoordString()
/*! 
   \brief  Cleans coordinate strings.

   \author fizzymagic
   \date   9/6/2003

   \param strCoordString [std::string&] - Cleaned string.
*/
// -------------------------------------------------------------------------
/*
void CLatLon::CleanCoordString(std::string& strCoordString)
{
   std::transform(strCoordString.begin(), strCoordString.end(), strCoordString.begin(), ::toupper);
   findandreplace(strCoordString, std::string("LATITUDE"), std::string(""));
   findandreplace(strCoordString, std::string("LONGITUDE"), std::string(""));
   findandreplace(strCoordString, std::string("LAT"), std::string(""));
   findandreplace(strCoordString, std::string("LON"), std::string(""));
   findandreplace(strCoordString, std::string("NORTH"), std::string("N"));
   findandreplace(strCoordString, std::string("SOUTH"), std::string("S"));
   findandreplace(strCoordString, std::string("EAST"), std::string("E"));
   findandreplace(strCoordString, std::string("WEST"), std::string("W"));
   findandreplace(strCoordString, std::string("WEST"), std::string("W"));
   std::string::iterator it;
   for (it = strCoordString.begin(); it != strCoordString.end(); it++) {
      if (!(::isalnum(*it) || *it == '-' || *it == '.' || *it == ',')) {
         *it = ' ';
      }
   }
   trim(strCoordString);
   findandreplace(strCoordString, std::string("  "), std::string(" "));
}
*/
// -------------------------------------------------------------------------
// METHOD:  CLatLon::GetZone()
/*! 
   \brief  Gets UTM zone of a point.

   \author fizzymagic
   \date   9/6/2003

   \return  [int] - UTM zone for this point.
*/
// -------------------------------------------------------------------------
int CLatLon::GetZone(void)
{
   int iZone = ((int) floor((m_Longitude + 180.)/6.)) % 60  + 1;

   // Special zones for places in northern Europe.
   if (m_Latitude > 56. && m_Latitude <= 64. 
      && m_Longitude > 3. && m_Longitude <= 12.) {
      iZone = 32;
   }

   if (m_Latitude > 72. && m_Latitude < 84.) {
      if ((m_Longitude >= 0.) && (m_Longitude < 9.)) iZone = 31;
      else if ((m_Longitude >= 9.) && (m_Longitude < 21.)) iZone = 33;
      else if ((m_Longitude >= 21.) && (m_Longitude < 33.)) iZone = 35;
      else if ((m_Longitude >= 33.) && (m_Longitude < 42.)) iZone = 37;
   }

   return iZone;
}

// -------------------------------------------------------------------------
// METHOD:  CLatLon::GetZoneLetter()
/*! 
   \brief  Gets UTM zone letter of a point.

   \author fizzymagic
   \date   9/6/2003

   \return [char] - UTM zone letter for this point.
*/
// -------------------------------------------------------------------------
char CLatLon::GetZoneLetter(void)
{
   char cZoneLetter;
   if (m_Latitude >= 72. && m_Latitude <= 84.) {
      cZoneLetter = 'X';
   }
   else {
      int iZoneLetter = floor(m_Latitude + 80.)/8;
      if (iZoneLetter >= 0 && iZoneLetter < m_strZoneLetters.size()) {
         cZoneLetter = m_strZoneLetters[iZoneLetter];
      }
      else {
         cZoneLetter = 'Z';
      }
   }
   return cZoneLetter;
}

// -------------------------------------------------------------------------
// METHOD:  CLatLon::ConvertUTM()
/*! 
   \brief  Converts UTM Zone, Zone Letter, Easting, and Northing to a 
           CLatLon point.

   \author fizzymagic
   \date   9/6/2003

   \return  [bool] - true if conversion was performed without error.

   \param iZone [int] - UTM Zone
   \param iZoneLetter [char] - UTM Zone Letter
   \param dEasting [double] - UTM Easting
   \param dNorthing [double] - UTM Northing

*/
// -------------------------------------------------------------------------
bool CLatLon::ConvertUTM(int iZone, char cZoneLetter, double dEasting, double dNorthing)
{
   const double k0 = 0.9996;
   const double dEastingOffset = 5.e5;
   const double dNorthingOffsetSouth = 1.e6;

   double dLatitude, dLongitude;

   double a = m_Ellipsoid.m_a;
   double flat = 1./m_Ellipsoid.m_fInv;
   double ecc2 = 2. * flat - flat * flat;
   double ecc12 = (ecc2)/(1. - ecc2);
   double e1 = (1. - sqrt(1. - ecc2))/(1. + sqrt(1. - ecc2));
   double N1, T1, C1, R1, D, M;
   double dCentralLongitude;
   double mu, phi1;
   double x, y;

   x = dEasting - dEastingOffset;
   y = dNorthing;
   cZoneLetter = toupper(cZoneLetter);
   if (m_strZoneLetters.find(cZoneLetter) == std::string::npos) return false;

   if ((cZoneLetter - 'N') < 0) {
      y -= dNorthingOffsetSouth;
   }

   dCentralLongitude = ((double) (iZone - 1) * 6. - 180. + 3.) * m_Deg2Rad;

   M = y / k0;
   mu = M / (a * (1. - ecc2/4. - 3.*ecc2*ecc2/64. - 5.*ecc2*ecc2*ecc2/256.));

   phi1 = mu + (3.*e1/2. - 27.*e1*e1*e1/32.) * sin(2.*mu) 
           + (21.*e1*e1/16. - 55.*e1*e1*e1*e1/32.) * sin(4.*mu)
           + (151.*e1*e1*e1/96.) * sin(6.*mu);

   N1 = a / sqrt(1. - ecc2 * sin(phi1)*sin(phi1));
   T1 = tan(phi1)*tan(phi1);
   C1 = ecc12 * cos(phi1)*cos(phi1);
   R1 = a * (1. - ecc2) / pow(1. - ecc2*sin(phi1)*sin(phi1), 1.5);
   D = x/(N1*k0);

   dLatitude = phi1 - (N1*tan(phi1)/R1) * (D*D/2. - (5. + 3.*T1 + 10.*C1 - 4.*C1*C1 - 9.*ecc12)*D*D*D*D/24.
             + (61. + 90.*T1 + 298.*C1 + 45.*T1*T1 - 252.*ecc12 - 3.*C1*C1)*D*D*D*D*D*D/720.);

   dLongitude = (D - (1.+ 2.*T1 + C1)*D*D*D/6. +(5.- 2.*C1 + 28.*T1 - 3.*C1*C1 + 8.*ecc12 + 24.*T1*T1)*D*D*D*D*D/120.)/cos(phi1);

   m_Latitude = dLatitude / m_Deg2Rad;
   m_Longitude = (dCentralLongitude + dLongitude) / m_Deg2Rad;

   return true;
}

// -------------------------------------------------------------------------
// METHOD:  CLatLon::ToCartesian()
/*! 
   \brief  Converts lat/lon into Cartesian coordinates using ellipsoid.

   \author fizzymagic
   \date   9/7/2003

   \return  [CCartesianCoord] - Cartesian coordinates in meters.
*/
// -------------------------------------------------------------------------
CCartesianCoord CLatLon::ToCartesian(void)
{
   double a = m_Ellipsoid.m_a;
   double flat = 1./m_Ellipsoid.m_fInv;
   double ecc2 = 2. * flat - flat * flat;
   double sinLat = sin(m_Latitude * m_Deg2Rad);
   double cosLat = cos(m_Latitude * m_Deg2Rad);
   double w = sqrt(1. - ecc2 * sinLat * sinLat);
   double r = a / w;

   double dLong = m_Longitude * m_Deg2Rad;
   double x = r * cos(dLong) * cosLat;
   double y = r * sin(dLong) * cosLat;
   double z = r * (1. - ecc2) * sinLat;

   return CCartesianCoord(x, y, z);
}

// -------------------------------------------------------------------------
// METHOD:  CLatLon::FromCartesian()
/*! 
   \brief  Converts Cartesian coordinates into lat/lon using ellipsoid.

   \author fizzymagic
   \date   9/7/2003

   \param C [CCartesianCoord&] - Cartesian coordinates of point.
*/
// -------------------------------------------------------------------------
void CLatLon::FromCartesian(CCartesianCoord& C)
{
   double a = m_Ellipsoid.m_a;
   double flat = 1./m_Ellipsoid.m_fInv;
   double ecc2 = 2. * flat - flat * flat;
   double r = a * ecc2;

   double p = sqrt(C.m_x*C.m_x + C.m_y*C.m_y);

   double dTmp = C.m_z / (p * (1. - ecc2));
   double dLast;

   do {
      dLast = dTmp;
      dTmp = C.m_z / (p - r / sqrt(1. + (1. - ecc2) * dTmp * dTmp));
   } while (fabs(dLast - dTmp) > EPSILON);

   m_Latitude = atan(dTmp) / m_Deg2Rad;
   m_Longitude = atan2(C.m_y, C.m_x) / m_Deg2Rad;
}

// -------------------------------------------------------------------------
// METHOD:  CLatLon::ToSphericalCartesian()
/*! 
   \brief  Converts lat/lon into Cartesian coordinates using spheroid.

   \author fizzymagic
   \date   9/7/2003

   \return  [CCartesianCoord] - Cartesian coordinates in meters.
*/
// -------------------------------------------------------------------------
CCartesianCoord CLatLon::ToSphericalCartesian(void)
{
      double dLat = m_Latitude * m_Deg2Rad;
      double dLong = m_Longitude * m_Deg2Rad;
      return CCartesianCoord(m_Radius * cos(dLong) * cos(dLat),
                             m_Radius * sin(dLong) * cos(dLat),
                             m_Radius * sin(dLat));
}

// -------------------------------------------------------------------------
// METHOD:  CLatLon::FromSphericalCartesian()
/*! 
   \brief  Converts Cartesian coordinates into lat/lon using spheroid.

   \author fizzymagic
   \date   9/7/2003

   \param C [CCartesianCoord&] - Cartesian coordinates of point.
*/
// -------------------------------------------------------------------------
void CLatLon::FromSphericalCartesian(CCartesianCoord& C)
{
   m_Latitude = atan2(C.m_z, sqrt(C.m_x*C.m_x + C.m_y*C.m_y)) / m_Deg2Rad;
   m_Longitude = atan2(C.m_y, C.m_x) / m_Deg2Rad;
}

// -------------------------------------------------------------------------
// METHOD:  CLatLon::SphericalCross()
/*! 
   \brief  Performs a cross-product between this and another CLatLon point
           using spheroidal approximation.

   \author fizzymagic
   \date   9/6/2003

   \return  [CCartesianCoord] - Cross-product vector (un-normalized).

   \param P [CLatLon&] - Point with which to perform cross-product.
*/
// -------------------------------------------------------------------------
CCartesianCoord CLatLon::SphericalCross(CLatLon& P)
{
   double dLat1 = m_Latitude * m_Deg2Rad;
   double dLong1 = m_Longitude * m_Deg2Rad;
   double dLat2 = P.m_Latitude * m_Deg2Rad;
   double dLong2 = P.m_Longitude * m_Deg2Rad;
   double dDeltaLat = dLat1 - dLat2;
   double dSumLat = dLat1 + dLat2;
   double dDeltaLong = (dLong1 - dLong2)/2.;
   double dAvgLong = (dLong1 + dLong2)/2.;

   return CCartesianCoord(sin(dSumLat) * cos(dAvgLong) * sin(dDeltaLong) 
                        - sin(dDeltaLat) * sin(dAvgLong) * cos(dDeltaLong),
                          sin(dDeltaLat) * cos(dAvgLong) * cos(dDeltaLong) 
                        + sin(dSumLat) * sin(dAvgLong) * sin(dDeltaLong),
                          cos(dLat1) * cos(dLat2) * sin(-2.*dDeltaLong));
}

/*
void findandreplace(std::string& strSource, std::string& strFind, std::string& strReplace)
{
   std::string::size_type i;
   while ((i = strSource.find(strFind)) != std::string::npos) {
      strSource.replace(i, strFind.length(), strReplace);
   }
}

void trim(std::string& strSource)
{
   std::reverse(strSource.begin(), strSource.end());
   std::string::iterator it = strSource.begin(), eit = strSource.end();
   while (it != eit && ::isspace(*it)) it++;
   strSource.erase(strSource.begin(), it);
   std::reverse(strSource.begin(), strSource.end());
   it = strSource.begin(), eit = strSource.end();
   while (it != eit && ::isspace(*it)) it++;
   strSource.erase(strSource.begin(), it);
}

*/