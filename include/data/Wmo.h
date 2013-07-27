/*
 *   Copyright 1995 University Corporation for Atmospheric Research
 *   See ../COPYRIGHT file for copying and redistribution conditions.
 */
/* $Id: grib1.h,v 1.5 1995/06/02 17:11:49 russ Exp $ */
/* Cochonne JC Berges Janvier 1997 */
/*
 * NMC "GRIB" Edition 1 Data structures, taken from WMO FM 92-X GRIB
 */

#pragma once

#define MAX_LIGNE_GRIB	2000

#define GRIB_ARB	100	/* Arbitrary size for unspecified lengths */
#define G1I_MISSING	255	/* Used for missing or undefined value for
				   1-byte quantities */
#define G2I_MISSING	65535	/* Used for missing or undefined value for
				   2-byte quantities */

typedef unsigned char g1int;		/* A GRIB 1-byte integer */
typedef unsigned char g2int[2];	/* A GRIB 2-byte integer */
typedef unsigned char g3int[3];	/* A GRIB 3-byte integer */
typedef unsigned char g3sint[3];	/* A GRIB signed 3-byte integer */
typedef unsigned char g4flt[4];	/* A GRIB 4-byte float */
typedef unsigned char g2sint[2];	/* A GRIB signed 2-byte integer */

extern float reel4();	/* g4flt to float */

/*
 + GRIB Table 6, Data Representation Types for Grid Description Section
 */
#define GRID_LL		0	/* Latitude/Longitude also called
				   Equidistant Cylindrical or Plate Carree */
#define GRID_MERCAT	1	/* Mercator Projection */
#define GRID_LAMBERT	3	/* Lambert Conformal, secant or tangent,
				   conical or bipolar (normal or oblique)
				   Projection */
#define GRID_GAU	4	/* Gaussian Latitude/Longitude */
#define GRID_POLARS	5	/* Polar Stereographic Projection */
#define	GRID_ALBERS	8	/* Albers equal-area, secant or tangent,
				   conical or bipolar, projection */
#define	GRID_RLL	10	/* Rotated latitude/longitude */
#define GRID_OLAMBERT	13	/* Oblique Lambert conformal, secant or
				   tangent, conical or bipolar, projection */
#define	GRID_RGAU	14	/* Rotated Gaussian latitude/longitude */
#define GRID_SLL	20	/* Stretched latitude/longitude */
#define GRID_SGAU	24	/* Stretched Gaussian latitude/longitude */
#define GRID_SRLL	30	/* Stretched and rotated latitude/longitude */
#define GRID_SRGAU	34	/* Stretched and rotated Gaussian
				   latitude/longitude */
#define GRID_SPH	50	/* Spherical Harmonic Coefficients */
#define GRID_RSPH	60	/* Rotated spherical harmonics */
#define GRID_SSPH	70	/* Stretched spherical harmonics */
#define GRID_SRSPH	80	/* Stretched and rotated spherical harmonics */
#define GRID_SPACEV	90	/* Space view perspective or orthographic */

/* Parameters for these are not defined in WMO or Stackpole GRIB documents */
#define GRID_GNOMON	2	/* Gnomonic Projection */
#define	GRID_UTM	6	/* Universal Transverse Mercator (UTM)
				   projection */
#define	GRID_SIMPOL	7	/* Simple polyconic projection */
#define	GRID_MILLER	9	/* Miller's cylindrical projection */

/* GRIB Table 7 -- Resolution and Component Flags */
#define RESCMP_DIRINC	0x80	/* bit on if directional increments given */
#define RESCMP_EARTH	0x40	/* bit on if earth assumed to be IAU 1965
				   oblate spheroid, off if sperical earth
				   assumed (radius=6367.47 km) */
#define RESCMP_UVRES	0x08	/* bit on if u- and v-components of vector
				   quantities resolved relative to the
				   defined grid in the direction of
				   increasing x and y (or i and j)
				   coordinates respectively, bit off if u-
				   and v-components of vector quantities
				   resolved relative to easterly and
				   northerly directions */

/* Grid Table 8 -- Scanning Mode Flag */
#define SCAN_I_MINUS	0x80	/* bit on if points scan in -i direction,
				   bit off if points scan in +i direction */
#define SCAN_J_PLUS	0x40	/* bit on if points scan in +j direction,
				   bit off if points scan in -j direction */
#define SCAN_J_CONSEC	0x20	/* bit on if adjacent points in j direction
				   are consecutive, bit off if adjacent
				   points in i direction are consecutive */

/*
 * lat/lon grid (or equidistant cylindrical, or Plate Carree),
 * used when grib1->gds->type is GRID_LL
 */
typedef struct
    {
    g2int ni;			/* number of points along a parallel */
    g2int nj;			/* number of points along a meridian */
    g3sint la1;			/* latitude of first grid point */
    g3sint lo1;			/* longitude of first grid point */
    g1int res_flags;		/* resolution and component flags (table 7) */
    g3sint la2;			/* latitude of last grid point */
    g3sint lo2;			/* longitude of last grid point */
    g2int di;			/* i direction increment */
    g2int dj;			/* j direction increment */
    g1int scan_mode;		/* scanning mode flags (table 8) */
    unsigned char reserved[4];
    union {			/* need not be present */
	g4flt vertc[GRIB_ARB];	/* vertical coordinate parameters */
	g2int npts[2*GRIB_ARB];	/* list of number of points in each row */
    } vn;
} grid_ll;

/*
 * rotated lat/lon grid (or equidistant cylindrical, or Plate Carree),
 * used when grib1->gds->type is GRID_RLL
 */
typedef struct
    {
    g2int ni;			/* number of points along a parallel */
    g2int nj;			/* number of points along a meridian */
    g3sint la1;			/* latitude of first grid point */
    g3sint lo1;			/* longitude of first grid point */
    g1int res_flags;		/* resolution and component flags (table 7) */
    g3sint la2;			/* latitude of last grid point */
    g3sint lo2;			/* longitude of last grid point */
    g2int di;			/* i direction increment */
    g2int dj;			/* j direction increment */
    g1int scan_mode;		/* scanning mode flags (table 8) */
    unsigned char reserved[4];
    g3sint lapole;		/* latitude of southern pole */
    g3sint lopole;		/* longitude of southern pole */
    g4flt angrot;		/* angle of rotation */
    union {			/* need not be present */
	g4flt vertc[GRIB_ARB];	/* vertical coordinate parameters */
	g2int npts[2*GRIB_ARB];	/* list of number of points in each row */
    } vn;
} grid_rll;

/*
 * stretched lat/lon grid (or equidistant cylindrical, or Plate Carree),
 * used when grib1->gds->type is GRID_SLL
 */
typedef struct
    {
    g2int ni;			/* number of points along a parallel */
    g2int nj;			/* number of points along a meridian */
    g3sint la1;			/* latitude of first grid point */
    g3sint lo1;			/* longitude of first grid point */
    g1int res_flags;		/* resolution and component flags (table 7) */
    g3sint la2;			/* latitude of last grid point */
    g3sint lo2;			/* longitude of last grid point */
    g2int di;			/* i direction increment */
    g2int dj;			/* j direction increment */
    g1int scan_mode;		/* scanning mode flags (table 8) */
    unsigned char reserved[4];
    g3sint lastr;		/* latitude of pole of stretching */
    g3sint lostr;		/* longitude of pole of stretching */
    g4flt stretch;		/* stretching factor */
    union {			/* need not be present */
	g4flt vertc[GRIB_ARB];	/* vertical coordinate parameters */
	g2int npts[2*GRIB_ARB];	/* list of number of points in each row */
    } vn;
} grid_sll;

/*
 * stretched and rotated lat/lon grid (or equidistant cylindrical, or Plate
 * Carree), * used when grib1->gds->type is GRID_SRLL
 */
typedef struct
    {
    g2int ni;			/* number of points along a parallel */
    g2int nj;			/* number of points along a meridian */
    g3sint la1;			/* latitude of first grid point */
    g3sint lo1;			/* longitude of first grid point */
    g1int res_flags;		/* resolution and component flags (table 7) */
    g3sint la2;			/* latitude of last grid point */
    g3sint lo2;			/* longitude of last grid point */
    g2int di;			/* i direction increment */
    g2int dj;			/* j direction increment */
    g1int scan_mode;		/* scanning mode flags (table 8) */
    unsigned char reserved[4];
    g3sint lapole;		/* latitude of southern pole */
    g3sint lopole;		/* longitude of southern pole */
    g4flt angrot;		/* angle of rotation */
    g3sint lastr;		/* latitude of pole of stretching */
    g3sint lostr;		/* longitude of pole of stretching */
    g4flt stretch;		/* stretching factor */
    union {			/* need not be present */
	g4flt vertc[GRIB_ARB];	/* vertical coordinate parameters */
	g2int npts[2*GRIB_ARB];	/* list of number of points in each row */
    } vn;
} grid_srll;

/*
 * Gaussian lat/lon grid, used when grib1->gds->type is GRID_GAU
 */
typedef struct
    {
    g2int ni;			/* number of points along a parallel */
    g2int nj;			/* number of points along a meridian */
    g3sint la1;			/* latitude of first grid point */
    g3sint lo1;			/* longitude of first grid point */
    g1int res_flags;		/* resolution and component flags (table 7) */
    g3sint la2;			/* latitude of last grid point */
    g3sint lo2;			/* longitude of last grid point */
    g2int di;			/* i direction increment */
    g2int n;			/* # of parallels between a pole and equator */
    g1int scan_mode;		/* scanning mode flags (table 8) */
    unsigned char reserved[4];
    union {			/* need not be present */
	g4flt vertc[GRIB_ARB];	/* vertical coordinate parameters */
	g2int npts[2*GRIB_ARB];	/* list of number of points in each row */
    } vn;
} grid_gau;

/*
 * Rotated Gaussian lat/lon grid, used when grib1->gds->type is GRID_RGAU
 */
typedef struct
    {
    g2int ni;			/* number of points along a parallel */
    g2int nj;			/* number of points along a meridian */
    g3sint la1;			/* latitude of first grid point */
    g3sint lo1;			/* longitude of first grid point */
    g1int res_flags;		/* resolution and component flags (table 7) */
    g3sint la2;			/* latitude of last grid point */
    g3sint lo2;			/* longitude of last grid point */
    g2int di;			/* i direction increment */
    g2int n;			/* # of parallels between a pole and equator */
    g1int scan_mode;		/* scanning mode flags (table 8) */
    unsigned char reserved[4];
    g3sint lapole;		/* latitude of southern pole */
    g3sint lopole;		/* longitude of southern pole */
    g4flt angrot;		/* angle of rotation */
    union {			/* need not be present */
	g4flt vertc[GRIB_ARB];	/* vertical coordinate parameters */
	g2int npts[2*GRIB_ARB];	/* list of number of points in each row */
    } vn;
} grid_rgau;

/*
 * Stretched Gaussian lat/lon grid, used when grib1->gds->type is GRID_SGAU
 */
typedef struct
    {
    g2int ni;			/* number of points along a parallel */
    g2int nj;			/* number of points along a meridian */
    g3sint la1;			/* latitude of first grid point */
    g3sint lo1;			/* longitude of first grid point */
    g1int res_flags;		/* resolution and component flags (table 7) */
    g3sint la2;			/* latitude of last grid point */
    g3sint lo2;			/* longitude of last grid point */
    g2int di;			/* i direction increment */
    g2int n;			/* # of parallels between a pole and equator */
    g1int scan_mode;		/* scanning mode flags (table 8) */
    unsigned char reserved[4];
    g3sint lastr;		/* latitude of pole of stretching */
    g3sint lostr;		/* longitude of pole of stretching */
    g4flt stretch;		/* stretching factor */
    union {			/* need not be present */
	g4flt vertc[GRIB_ARB];	/* vertical coordinate parameters */
	g2int npts[2*GRIB_ARB];	/* list of number of points in each row */
    } vn;
} grid_sgau;

/*
 * Stretched and rotated Gaussian lat/lon grid, used when grib1->gds->type
 * is GRID_SRGAU
 */
typedef struct
    {
    g2int ni;			/* number of points along a parallel */
    g2int nj;			/* number of points along a meridian */
    g3sint la1;			/* latitude of first grid point */
    g3sint lo1;			/* longitude of first grid point */
    g1int res_flags;		/* resolution and component flags (table 7) */
    g3sint la2;			/* latitude of last grid point */
    g3sint lo2;			/* longitude of last grid point */
    g2int di;			/* i direction increment */
    g2int n;			/* # of parallels between a pole and equator */
    g1int scan_mode;		/* scanning mode flags (table 8) */
    unsigned char reserved[4];
    g3sint lapole;		/* latitude of southern pole */
    g3sint lopole;		/* longitude of southern pole */
    g4flt angrot;		/* angle of rotation */
    g3sint lastr;		/* latitude of pole of stretching */
    g3sint lostr;		/* longitude of pole of stretching */
    g4flt stretch;		/* stretching factor */
    union {			/* need not be present */
	g4flt vertc[GRIB_ARB];	/* vertical coordinate parameters */
	g2int npts[2*GRIB_ARB];	/* list of number of points in each row */
    } vn;
} grid_srgau;

/*
 * Spherical harmonic coefficients, used when grib1->gds->type is GRID_SPH
 */
typedef struct
    {
    g2int j;			/* J pentagonal resolution parameter */
    g2int k;			/* K pentagonal resolution parameter */
    g2int m;			/* M pentagonal resolution parameter */
    g1int type;			/* Representation type (table 9) */
    g1int mode;			/* Representation mode (table 10) */
    unsigned char reserved[18];
    union {			/* need not be present */
	g4flt vertc[GRIB_ARB];	/* vertical coordinate parameters */
	g2int npts[2*GRIB_ARB];	/* list of number of points in each row */
    } vn;
} grid_sph;

/*
 * Rotated spherical harmonic coefficients, used when
 * grib1->gds->type is GRID_RSPH
 */
typedef struct
    {
    g2int j;			/* J pentagonal resolution parameter */
    g2int k;			/* K pentagonal resolution parameter */
    g2int m;			/* M pentagonal resolution parameter */
    g1int type;			/* Representation type (table 9) */
    g1int mode;			/* Representation mode (table 10) */
    unsigned char reserved[18];
    g3sint lapole;		/* latitude of southern pole */
    g3sint lopole;		/* longitude of southern pole */
    g4flt angrot;		/* angle of rotation */
    union {			/* need not be present */
	g4flt vertc[GRIB_ARB];	/* vertical coordinate parameters */
	g2int npts[2*GRIB_ARB];	/* list of number of points in each row */
    } vn;
} grid_rsph;

/*
 * Stretched spherical harmonic coefficients, used when
 * grib1->gds->type is GRID_SSPH
 */
typedef struct
    {
    g2int j;			/* J pentagonal resolution parameter */
    g2int k;			/* K pentagonal resolution parameter */
    g2int m;			/* M pentagonal resolution parameter */
    g1int type;			/* Representation type (table 9) */
    g1int mode;			/* Representation mode (table 10) */
    unsigned char reserved[18];
    g3sint lastr;		/* latitude of pole of stretching */
    g3sint lostr;		/* longitude of pole of stretching */
    g4flt stretch;		/* stretching factor */
    union {			/* need not be present */
	g4flt vertc[GRIB_ARB];	/* vertical coordinate parameters */
	g2int npts[2*GRIB_ARB];	/* list of number of points in each row */
    } vn;
} grid_ssph;

/*
 * Stretched and rotated spherical harmonic coefficients, used when
 * grib1->gds->type is GRID_SRSPH
 */
typedef struct
    {
    g2int j;			/* J pentagonal resolution parameter */
    g2int k;			/* K pentagonal resolution parameter */
    g2int m;			/* M pentagonal resolution parameter */
    g1int type;			/* Representation type (table 9) */
    g1int mode;			/* Representation mode (table 10) */
    unsigned char reserved[18];
    g3sint lapole;		/* latitude of southern pole */
    g3sint lopole;		/* longitude of southern pole */
    g4flt angrot;		/* angle of rotation */
    g3sint lastr;		/* latitude of pole of stretching */
    g3sint lostr;		/* longitude of pole of stretching */
    g4flt stretch;		/* stretching factor */
    union {			/* need not be present */
	g4flt vertc[GRIB_ARB];	/* vertical coordinate parameters */
	g2int npts[2*GRIB_ARB];	/* list of number of points in each row */
    } vn;
} grid_srsph;

/*
 * Polar stereographic grid, used when grib1->gds->type is GRID_POLARS
 */
typedef struct
    {
    g2int nx;			/* number of points along X-axis */
    g2int ny;			/* number of points along Y-axis */
    g3sint la1;			/* latitude of first grid point */
    g3sint lo1;			/* longitude of first grid point */
    g1int res_flags;		/* resolution and component flags (table 7) */
    g3sint lov;			/* orientation of the grid */
    g3int dx;			/* X-direction grid length */
    g3int dy;			/* Y-direction grid length */
    g1int pole;			/* Projection center flag */
    g1int scan_mode;		/* scanning mode flags (table 8) */
    unsigned char reserved[4];
    union {			/* need not be present */
	g4flt vertc[GRIB_ARB];	/* vertical coordinate parameters */
	g2int npts[2*GRIB_ARB];	/* list of number of points in each row */
    } vn;
} grid_polars;

/*
 * Gnomonic grid, used when grib1->gds->type is GRID_GNOMON
 */
typedef struct
    {
    g2int nx;			/* number of points along X-axis */
    g2int ny;			/* number of points along Y-axis */
    g3sint la1;			/* latitude of first grid point */
    g3sint lo1;			/* longitude of first grid point */
    g1int res_flags;		/* resolution and component flags (table 7) */
    g3sint lov;			/* orientation of the grid */
    g3int dx;			/* X-direction grid length */
    g3int dy;			/* Y-direction grid length */
    g1int pole;		/* Projection center flag */
    g1int scan_mode;		/* scanning mode flags (table 8) */
    unsigned char reserved[4];
    union {			/* need not be present */
	g4flt vertc[GRIB_ARB];	/* vertical coordinate parameters */
	g2int npts[2*GRIB_ARB];	/* list of number of points in each row */
    } vn;
} grid_gnomon;

/*
 * Mercator grid, used when grib1->gds->type is GRID_MERCAT
 */
typedef struct grid_mercator {
    g2int ni;			/* number of points along a parallel */
    g2int nj;			/* number of points along a meridian */
    g3sint la1;			/* latitude of first grid point */
    g3sint lo1;			/* longitude of first grid point */
    g1int res_flags;		/* resolution and component flags (table 7) */
    g3sint la2;			/* latitude of last grid point */
    g3sint lo2;			/* longitude of last grid point */
    g3sint latin;		/* latitude at which the Mercator projection
				   cylinder intersects the Earth */
    unsigned char reserved;
    g1int scan_mode;		/* scanning mode flags (table 8) */
    g3int di;			/* longitudinal direction grid length */
    g3int dj;			/* latitudinal direction grid length */
    unsigned char reserved1[8];
    union {			/* need not be present */
	g4flt vertc[GRIB_ARB];	/* vertical coordinate parameters */
	g2int npts[2*GRIB_ARB];	/* list of number of points in each row */
    } vn;
} grid_mercator;

/*
 * Lambert conformal, secant or tangent, conic or bi-polar (normal or oblique),
 * used when  grib1->gds->type is GRID_LAMBERT
 */
typedef struct
    {
    g2int nx;			/* number of points along X-axis */
    g2int ny;			/* number of points along Y-axis */
    g3sint la1;			/* latitude of first grid point */
    g3sint lo1;			/* longitude of first grid point */
    g1int res_flags;		/* resolution and component flags (table 7) */
    g3sint lov;			/* orientation of the grid */
    g3int dx;			/* X-direction grid length */
    g3int dy;			/* Y-direction grid length */
    g1int pole;		/* Projection center flag */
    g1int scan_mode;		/* scanning mode flags (table 8) */
    g3sint latin1;		/* First latitude from the pole at which the
				   secant cone cuts the sphere */
    g3sint latin2;		/* Second latitude from the pole at which the
				   secant cone cuts the sphere */
    g3sint splat;		/* Latitude of the southern pole */
    g3sint splon;		/* Longitude of the southern pole */
    unsigned char reserved[2];
    union {			/* need not be present */
	g4flt vertc[GRIB_ARB];	/* vertical coordinate parameters */
	g2int npts[2*GRIB_ARB];	/* list of number of points in each row */
    } vn;
} grid_lambert;

/*
 * Oblique Lambert conformal uses same fields as normal Lambert conformal,
 * used when grib1->gds->type is GRID_OLAMBERT
 */
typedef struct
    {
    g2int nx;			/* number of points along X-axis */
    g2int ny;			/* number of points along Y-axis */
    g3sint la1;			/* latitude of first grid point */
    g3sint lo1;			/* longitude of first grid point */
    g1int res_flags;		/* resolution and component flags (table 7) */
    g3sint lov;			/* orientation of the grid */
    g3int dx;			/* X-direction grid length */
    g3int dy;			/* Y-direction grid length */
    g1int pole;		/* Projection center flag */
    g1int scan_mode;		/* scanning mode flags (table 8) */
    g3sint latin1;		/* First latitude from the pole at which the
				   secant cone cuts the sphere */
    g3sint latin2;		/* Second latitude from the pole at which the
				   secant cone cuts the sphere */
    g3sint splat;		/* Latitude of the southern pole */
    g3sint splon;		/* Longitude of the southern pole */
    unsigned char reserved[2];
    union {			/* need not be present */
	g4flt vertc[GRIB_ARB];	/* vertical coordinate parameters */
	g2int npts[2*GRIB_ARB];	/* list of number of points in each row */
    } vn;
} grid_olambert;

/*
 * Albers equal-area, secant or tangent, conic or bi-polar (normal or oblique),
 * used when  grib1->gds->type is GRID_ALBERS
 */
typedef struct
    {
    g2int nx;			/* number of points along X-axis */
    g2int ny;			/* number of points along Y-axis */
    g3sint la1;			/* latitude of first grid point */
    g3sint lo1;			/* longitude of first grid point */
    g1int res_flags;		/* resolution and component flags (table 7) */
    g3sint lov;			/* orientation of the grid */
    g3int dx;			/* X-direction grid length */
    g3int dy;			/* Y-direction grid length */
    g1int pole;		/* Projection center flag */
    g1int scan_mode;		/* scanning mode flags (table 8) */
    g3sint latin1;		/* First latitude from the pole at which the
				   secant cone cuts the sphere */
    g3sint latin2;		/* Second latitude from the pole at which the
				   secant cone cuts the sphere */
    g3sint splat;		/* Latitude of the southern pole */
    g3sint splon;		/* Longitude of the southern pole */
    unsigned char reserved[2];
    union {			/* need not be present */
	g4flt vertc[GRIB_ARB];	/* vertical coordinate parameters */
	g2int npts[2*GRIB_ARB];	/* list of number of points in each row */
    } vn;
} grid_albers;

/*
 * Space view perspective or orthographic,
 * used when  grib1->gds->type is GRID_SPACEV
 */
typedef struct
    {
    g2int nx;			/* number of points along X-axis (columns) */
    g2int ny;			/* number of points along Y-axis (rows or
				   lines) */
    g3sint lap;			/* latitude of sub-satellite point */
    g3sint lop;			/* longitude of sub-satellite point */
    g1int res_flags;		/* resolution and component flags (table 7) */
    g3int dx;			/* apparent diameter of earth in grid lengths,
				   in x direction */
    g3int dy;			/* apparent diameter of earth in grid lengths,
				   in y direction */
    g2int xp;			/* Xp - X-coordinate of sub satellite point */
    g2int yp;			/* Y-coordinate of sub-satellite point */
    g1int scan_mode;		/* scanning mode flags (table 8) */
    g3int orient;		/* orientation of the grid; i.e., the angle in
				   millidegrees between the increasing y axis
				   and the meridian of the sub-satellite point
				   in the direction of increasing latitude */
    g3int nr;			/* altitude of the camera from the earth's
				   center, measured in units of the earth's
				   (equatorial) radius */
    g2int xo;			/* X-coordinate of origin of sector image */
    g2int yo;			/* Y-coordinate of origin of sector image */
    unsigned char reserved[6];
    union {			/* need not be present */
	g4flt vertc[GRIB_ARB];	/* vertical coordinate parameters */
	g2int npts[2*GRIB_ARB];	/* list of number of points in each row */
    } vn;
} grid_spacev;


/*
 * Grid Description Section, a raw mapping of the GRIB bytes
 */
typedef struct gds {
    g3int len;			/* length of Grid Description Section */
    g1int nv;			/* number of vertical coordinate parameters */
    g1int pv;			/* byte number of the list of vertical coords,
				 * if present, or list of points in each row,
				 * for quasi-regular grids */
    g1int type;			/* representation type, from GRIB table 6 */
    union {
        grid_ll  		ll; /* type GRID_LL */
        grid_rll  		rll; /* type GRID_RLL */
        grid_sll  		sll; /* type GRID_SLL */
        grid_srll  		srll; /* type GRID_SRLL */
        grid_gau  		gau; /* type GRID_GAU */
        grid_rgau  		rgau; /* type GRID_RGAU */
        grid_sgau  		sgau; /* type GRID_SGAU */
        grid_srgau  		srgau; /* type GRID_SRGAU */
        grid_sph  		sph; /* type GRID_SPH */
        grid_rsph  		rsph; /* type GRID_RSPH */
        grid_ssph  		ssph; /* type GRID_SSPH */
        grid_srsph  		srsph; /* type GRID_SRSPH */
        grid_polars 		polars;	/* type GRID_POLARS */
	grid_gnomon		gnomon;	/* type GRID_GNOMON */
        grid_mercator 		mercator; /* type GRID_MERCAT */
        grid_lambert  		lambert; /* type GRID_LAMBERT */
        grid_olambert  		olambert; /* type GRID_OLAMBERT */
	grid_albers		albers;	/* type GRID_ALBERS */
	grid_spacev		spacev;	/* type GRID_SPACEV */
	/* Not well-specified in WMO or Stackpole GRIB documents: */
	/* grid_utm		utm; */	        /* type GRID_UTM */
	/* grid_simpol		simpol; */	/* type GRID_SIMPOL */
	/* grid_miller		miller; */	/* type GRID_MILLER */
    } grid;
} gds;	

#define MAX_GRIB_SIZE	20000	/* The biggest we've seen on HRS is 6045 */
#define HAS_GDS		0x80
#define HAS_BMS		0x40    
#define NONCATALOGED_GRID 255	/* indicates grid is defined by GDS */

/*
 * Indicator Section
 */
typedef struct ids {
    char grib[4];		/* 'G', 'R', 'I', 'B' */
    g3int len;			/* length of entire GRIB message */
    g1int edition;		/* GRIB edition number */
} ids;

/*
 * Product Definition Section
 */
typedef struct pds {
    g3int len;			/* length of section in bytes */
    g1int table_version;	/* GRIB tables version number */
    g1int center;		/* id of center */
    g1int model;		/* model id, allocated by center */
    g1int grid;			/* grid id, allocated by center */
    g1int db_flg;		/* whether GDS, BMS included */
    g1int param;		/* parameter id, from GRIB table 2 */
    g1int level_flg;		/* type of level, from GRIB table 3 */
    union {			/* height, pressure, etc of levels (table 3) */
	g2int lev;		/* one level from 2 bytes */
	g1int levs[2];		/* two 1-byte levels */
    } level;
    g1int year;			/* reference time of data */
    g1int month;
    g1int day;
    g1int hour;
    g1int minute;
    g1int tunit;		/* unit of time range, from GRIB table 4 */
    g1int tr[2];		/* periods of time or interval */
    g1int tr_flg;		/* time range indicator, from GRIB table 5 */
    g2int avg;			/* number in average, if any */
    g1int missing;		/* number missing from averages or accums */
    g1int century;		/* 20 for years 1901-2000 */
    g1int subcenter;		/* reserved in GRIB1 standard */
    g2sint scale10;		/* (signed) units decimal scale factor */
    unsigned char reserved1[12];	/* reserved; need not be present */
    unsigned char reserved2[GRIB_ARB]; /* reserved for local center use;
				   need not be present */
} pds;

/*
 * Bit Map Section
 */
typedef struct bms {
    g3int len;			/* Length of section in bytes */
    g1int nbits;		/* Number of unused bits at end of section */
    g2int map_flg;		/* 0 if bit map follows, otherwise catalogued
				   bit map from center */
    unsigned char bits[GRIB_ARB];	/* the bit map */
} bms;


#define BDS_KIND	0x80	/* From GRIB Table 11, kind of data bit.  0
				   for grid-point data, 1 for spherical
				   harmonic coefficients */
#define BDS_PACKING	0x40	/* From GRIB Table 11, kind of packing bit.
				   0 for simple packing, 1 for complex
				   (spherical harmonics) or second-order
				   (grid) packing */
#define BDS_DATATYPE	0x20	/* From GRIB Table 11, data type bit.  0 if
				   floating-point values, 1 if integer
				   values */
#define BDS_MORE_FLAGS	0x10	/* From GRIB Table 11, additional flags bit.
				   0 if no additional flags, 1 if more flag
				   bits for grid-point data second-order
				   packing */
#define BDS_MATRIX	0x40	/* From GRIB Table 11, matrix values.  0 for
				   single datum at each grid point, 1 for
				   matrix of values at each grid point */
#define BDS_SECONDARY	0x20	/* From GRIB Table 11, secondary bit maps.
				   0 if no secondary bit maps, 1 if
				   secondary bit maps present */
#define BDS_WIDTHS	0x10	/* From GRIB Table 11, widths flag.  0 if
				   second-order values have constant width,
				   1 second-order values have different
				   widths */

#define BDS_MF_NON_STD			0x08	/* used by météo France to identify non std 2nd order packing */
#define BDS_MF_BOUSTRO			0x04	/* used by météo France to identify balayage boustrophédonique ;-) */
#define BDS_MF_SPC_DIFF_ORDER	0x03	/* used by météo France to enable différentiation spatiale (contient l'ordre) */

/*
 * Binary Data Section
 */
typedef struct bds {
    g3int len;			/* Length of section in bytes */
    g1int flg;			/* High 4 bits are flag from GRIB table 11.
				   Low 4 bits are no. of unused bits at end. */
    g2sint scale;		/* (signed) scale factor */
    g4flt ref;			/* Reference value (min of packed values) */
    g1int bits;			/* Number of bits containing each packed val */
    unsigned char data[GRIB_ARB]; /* The packed data */
} bds;

// second order packing bds
typedef struct {
	g2int N1;
	g1int ext_flg;
	g2int N2;
	g2int P1;
	g2int P2;
	g1int reserved;
    unsigned char data[GRIB_ARB]; /* The packed data */
} bds_2nd;

// second order packing bds Météo France
typedef struct {
	g1int nbits_nb_b_seq;
	g1int nbits_nb_p_seq;
	g2int ns_rg;
    unsigned char data[GRIB_ARB]; /* The packed data */
} bds_2nd_MF;

//#define PARM_RESERVED	0	/* Reserved */
//#define PARM_PRESSURE	1	/* Pressure, Pa */
//#define PARM_PMSL	2	/* Pressure reduced to MSL, Pa */
//#define PARM_PTND	3	/* Pressure tendency, Pa/s */
//#define PARM_GPT	6	/* Geopotential, m2/s2 */
//#define PARM_GPT_HGT	7	/* Geopotential height, gpm */
//#define PARM_GEOM_HGT	8	/* Geometric height, m */

//#define PARM_HSTDV	9	/* Standard deviation of height, m */
//#define PARM_TOTOZ	10	/* Total Ozone, Dobsons */
//#define PARM_TEMP	11	/* Temperature, deg. K */
//#define PARM_VTEMP	12	/* Virtual temperature, deg. K */
//#define PARM_POT_TEMP	13	/* Potential temperature, deg. K */
//#define PARM_APOT_TEMP	14	/* Pseudo-adiabatic potential temperature, deg. K */
//#define PARM_MAX_TEMP	15	/* Maximum temperature, deg. K */
//#define PARM_MIN_TEMP	16	/* Minimum temperature, deg. K */
//#define PARM_DP_TEMP	17	/* Dew point temperature, deg. K */
//#define PARM_DP_DEP	18	/* Dew point depression (or deficit), deg. K */
//#define PARM_LAPSE	19	/* Lapse rate, deg. K/m */

//#define PARM_VIS	20	/* Visibility, m */

//#define PARM_RAD1	21	/* Radar Spectra, direction and frequency */
//#define PARM_RAD2	22	/* Radar Spectra, direction and radial num */
//#define PARM_RAD3	23	/* Radar Spectra, radial num and radial num */
//#define PARM_TOZ	24	/* Total Ozone */
//#define PARM_TANOM	25	/* Temperature anomaly, deg. K */
//#define PARM_PANOM	26	/* Pressure anomaly, Pa */
//#define PARM_ZANOM	27	/* Geopotential height anomaly, gpm */
//#define PARM_WAV1	28	/* Wave Spectra, direction and frequency */
//#define PARM_WAV2	29	/* Wave Spectra, direction and radial num */
//#define PARM_WAV3	30	/* Wave Spectra, radial num and radial num */
//#define PARM_WND_DIR	31	/* Wind direction, deg. true */
//#define PARM_WND_SPEED	32	/* Wind speed, m/s */
//#define PARM_U_WIND	33	/* u-component of wind, m/s */
//#define PARM_V_WIND	34	/* v-component of wind, m/s */
//#define PARM_STRM_FUNC	35	/* Stream function, m2/s */
//#define PARM_VPOT	36	/* Velocity potential, m2/s */

//#define PARM_MNTSF	37	/* Montgomery stream function, m2/s2 */

//#define PARM_SIG_VEL	38	/* Sigma coord. vertical velocity, /s */
//#define PARM_VERT_VEL	39	/* Pressure Vertical velocity, Pa/s */
//#define PARM_GEOM_VEL	40	/* Geometric Vertical velocity, m/s */
//#define PARM_ABS_VOR	41	/* Absolute vorticity, /s */
//#define PARM_ABS_DIV	42	/* Absolute divergence, /s */
//#define PARM_REL_VOR	43	/* Relative vorticity, /s */
//#define PARM_REL_DIV	44	/* Relative divergence, /s */
//#define PARM_U_SHR	45	/* Vertical u-component shear, /s */
//#define PARM_V_SHR	46	/* Vertical v-component shear, /s */
//#define PARM_CRNT_DIR	47	/* Direction of current, deg. true */
//#define PARM_CRNT_SPD	48	/* Speed of current, m/s */
//#define PARM_U_CRNT	49	/* u-component of current, m/s */
//#define PARM_V_CRNT	50	/* v-component of current, m/s */
//#define PARM_SPEC_HUM	51	/* Specific humidity, kg/kg */
//#define PARM_REL_HUM	52	/* Relative humidity, % */
//#define PARM_HUM_MIX	53	/* Humidity mixing ratio, kg/kg */
//#define PARM_PR_WATER	54	/* Precipitable water, kg/m2 */
//#define PARM_VAP_PR	55	/* Vapor pressure, Pa */
//#define PARM_SAT_DEF	56	/* Saturation deficit, Pa */
//#define PARM_EVAP	57	/* Evaporation, kg/m2 */

//#define PARM_C_ICE	58	/* Cloud Ice, kg/m2 */

//#define PARM_PRECIP_RT	59	/* Precipitation rate, kg/m2/s */
//#define PARM_THND_PROB	60	/* Thunderstorm probability, % */
//#define PARM_PRECIP_TOT	61	/* Total precipitation, kg/m2 */
//#define PARM_PRECIP_LS	62	/* Large scale precipitation, kg/m2 */
//#define PARM_PRECIP_CN	63	/* Convective precipitation, kg/m2 */
//#define PARM_SNOW_RT	64	/* Snowfall rate water equivalent, kg/m2s */
//#define PARM_SNOW_WAT	65	/* Water equiv. of accum. snow depth, kg/m2 */
//#define PARM_SNOW	66	/* Snow depth, m */
//#define PARM_MIXED_DPTH	67	/* Mixed layer depth, m */
//#define PARM_TT_DEPTH	68	/* Transient thermocline depth, m */
//#define PARM_MT_DEPTH	69	/* Main thermocline depth, m */
//#define PARM_MTD_ANOM	70	/* Main thermocline anomaly, m */
//#define PARM_CLOUD	71	/* Total cloud cover, % */
//#define PARM_CLOUD_CN	72	/* Convective cloud cover, % */
//#define PARM_CLOUD_LOW	73	/* Low cloud cover, % */
//#define PARM_CLOUD_MED	74	/* Medium cloud cover, % */
//#define PARM_CLOUD_HI	75	/* High cloud cover, % */
//#define PARM_CLOUD_WAT	76	/* Cloud water, kg/m2 */

//#define PARM_SNO_C	78	/* Convective snow, kg/m2 */
//#define PARM_SNO_L	79	/* Large scale snow, kg/m2 */

//#define PARM_SEA_TEMP	80	/* sea temperature in degrees K */
//#define PARM_LAND_MASK	81	/* Land-sea mask (1=land; 0=sea), 1/0 */
//#define PARM_SEA_MEAN	82	/* Deviation of sea level from mean, m */
//#define PARM_SRF_RN	83	/* Surface roughness, m */
//#define PARM_ALBEDO	84	/* Albedo, % */
//#define PARM_SOIL_TEMP	85	/* Soil temperature, deg. K */
//#define PARM_SOIL_MST	86	/* Soil moisture content, kg/m2 */
//#define PARM_VEG	87	/* Vegetation, % */
//#define PARM_SAL	88	/* Salinity, kg/kg */
//#define PARM_DENS	89	/* Density, kg/m3 */

//#define PARM_WATR	90	/* Water runoff, kg/m2 */

//#define PARM_ICE_CONC	91	/* Ice concentration (ice=l; no ice=O), 1/0 */
//#define PARM_ICE_THICK	92	/* Ice thickness, m */
//#define PARM_ICE_DIR	93	/* Direction of ice drift, deg. true */
//#define PARM_ICE_SPD	94	/* Speed of ice drift, m/s */
//#define PARM_ICE_U	95	/* u-component of ice drift, m/s */
//#define PARM_ICE_V	96	/* v-component of ice drift, m/s */
//#define PARM_ICE_GROWTH	97	/* Ice growth, m */
//#define PARM_ICE_DIV	98	/* Ice divergence, /s */

//#define PARM_SNO_M	99	/* Snow melt, kg/m2 */

//#define PARM_WAVE_HGT	100	/* Significant height of combined wind waves and swell, m */
//#define PARM_SEA_DIR	101	/* Direction of wind waves, deg. true */
//#define PARM_SEA_HGT	102	/* Significant height of wind waves, m */
//#define PARM_SEA_PER	103	/* Mean period of wind waves, s */
//#define PARM_SWELL_DIR	104	/* Direction of swell waves, deg. true */
//#define PARM_SWELL_HGT	105	/* Significant height of swell waves, m */
//#define PARM_SWELL_PER	106	/* Mean period of swell waves, s */
//#define PARM_WAVE_DIR	107	/* Primary wave direction, deg. true */
//#define PARM_WAVE_PER	108	/* Primary wave mean period, s */
//#define PARM_WAVE2_DIR	109	/* Secondary wave direction, deg. true */
//#define PARM_WAVE2_PER	110	/* Secondary wave mean period, s */
//#define PARM_RDN_SWSRF	111	/* Net shortwave radiation (surface), W/m2 */
//#define PARM_RDN_LWSRF	112	/* Net longwave radiation (surface), W/m2 */
//#define PARM_RDN_SWTOP	113	/* Net shortwave radiation (top of atmos.), W/m2 */
//#define PARM_RDN_LWTOP	114	/* Net longwave radiation (top of atmos.), W/m2 */
//#define PARM_RDN_LW	115	/* Long wave radiation, W/m2 */
//#define PARM_RDN_SW	116	/* Short wave radiation, W/m2 */
//#define PARM_RDN_GLBL	117	/* Global radiation, W/m2 */
//#define PARM_LAT_HT	121	/* Latent heat flux, W/m2 */
//#define PARM_SEN_HT	122	/* Sensible heat flux, W/m2 */
//#define PARM_BL_DISS	123	/* Boundary layer dissipation, W/m2 */

//#define PARM_U_FLX	124	/* Momentum flux, u component, N/m2 */
//#define PARM_V_FLX	125	/* Momentum flux, v component, N/m2 */
//#define PARM_WMIXE	126	/* Wind mixing energy, J */

//#define PARM_IMAGE	127	/* Image data */
//#define PARM_MSLSA	128	/* Mean sea level pressure (std. atms. reduction), Pa */
//#define PARM_PM		129	/* Mean sea level pressure (MAPS system reduction), Pa */
//#define PARM_MSLET	130	/* Mean sea level pressure (ETA model reduction), Pa */
//#define PARM_LIFT_INDX	131	/* Surface lifted index, Deg. K */
//#define PARM_LIFT_INDX4	132	/* Best (4 layer) lifted index, Deg. K */
//#define PARM_K_INDX	133	/* K index, Deg. K */
//#define PARM_SW_INDX	134	/* Sweat index, Deg. K */
//#define PARM_HM_DIV	135	/* Horizontal moisture divergence, kg/kg/s */
//#define PARM_VERT_SSHR	136	/* Vertical speed shear, /s */
//#define PARM_TSLSA	137	/* 3-hr pressure tendency, Std. Atmos. Reduction, Pa/s */
//#define PARM_BVF_2	138	/* Brunt-Vaisala frequency (squared), 1/s2 */
//#define PARM_PV_MW	139	/* Potential vorticity (density weighted), 1/s/m */
//#define PARM_CRAIN	140	/* Categorical rain  (yes=1; no=0), non-dim */
//#define PARM_CFRZRN	141	/* Categorical freezing rain  (yes=1; no=0), non-dim */
//#define PARM_CICEPL	142	/* Categorical ice pellets  (yes=1; no=0), non-dim */
//#define PARM_CSNOW	143	/* Categorical snow  (yes=1; no=0), non-dim */
//#define PARM_COVMZ	150	/* Covariance between meridional and zonal components of wind, m2/s2 */
//#define PARM_COVTZ	151	/* Covariance between temperature and zonal component of wind, K*m/s */
//#define PARM_COVTM	152	/* Covariance between temperature and meridional component of wind, K*m/s */
//#define PARM_GFLUX	155	/* Ground Heat Flux, W/m2 */
//#define PARM_CIN	156	/* Convective inhibition, J/kg */
//#define PARM_CAPE	157	/* Convective Available Potential Energy, J/kg */
//#define PARM_TKE	158	/* Turbulent Kinetic Energy, J/kg */
//#define PARM_CONDP	159	/* Condensation pressure of parcel lifted from indicated surface, Pa */
//#define PARM_CSUSF	160	/* Clear Sky Upward Solar Flux, W/m2 */
//#define PARM_CSDSF	161	/* Clear Sky Downward Solar Flux, W/m2 */
//#define PARM_CSULF	162	/* Clear Sky upward long wave flux, W/m2 */
//#define PARM_CSDLF	163	/* Clear Sky downward long wave flux, W/m2 */
//#define PARM_CFNSF	164	/* Cloud forcing net solar flux, W/m2 */
//#define PARM_CFNLF	165	/* Cloud forcing net long wave flux, W/m2 */
//#define PARM_VBDSF	166	/* Visible beam downward solar flux, W/m2 */
//#define PARM_VDDSF	167	/* Visible diffuse downward solar flux, W/m2 */
//#define PARM_NBDSF	168	/* Near IR beam downward solar flux, W/m2 */
//#define PARM_NDDSF	169	/* Near IR diffuse downward solar flux, W/m2 */
//#define PARM_M_FLX	172	/* Momentum flux, N/m2 */
//#define PARM_LMH	173	/* Mass point model surface, non-dim */
//#define PARM_LMV	174	/* Velocity point model surface, non-dim */
//#define PARM_MLYNO	175	/* Model layer number (from bottom up), non-dim */
//#define PARM_NLAT	176	/* latitude (-90 to +90), deg */
//#define PARM_ELON	177	/* east longitude (0-360), deg */
//#define PARM_LPS_X	181	/* x-gradient of log pressure, 1/m */
//#define PARM_LPS_Y	182	/* y-gradient of log pressure, 1/m */
//#define PARM_HGT_X	183	/* x-gradient of height, m/m */
//#define PARM_HGT_Y	184	/* y-gradient of height, m/m */
//#define PARM_HELC	190	/* Storm relative helicity, m**2/s**2 */
//#define PARM_USTM	196	/* u-component of storm motion, m/s */
//#define PARM_VSTM	197	/* v-component of storm motion, m/s */
//#define PARM_NOICE_WAT	201	/* Ice-free water surface, % */

//#define PARM_DSWRF	204	/* downward short wave rad. flux, W/m2 */
//#define PARM_DLWRF	205	/* downward long wave rad. flux, W/m2 */
//#define PARM_UVPI	206	/* Ultra violet potential index, W/m2 */
//#define PARM_MSTR_AVL	207	/* Moisture availability, % */
//#define PARM_XCHG_COF	208	/* Exchange coefficient, (kg/m3)(m/s) */
//#define PARM_NMIX_LYRS	209	/* No. of mixed layers next to surface, integer */

//#define PARM_USWRF	211	/* upward short wave rad. flux, W/m2 */
//#define PARM_ULWRF	212	/* upward long wave rad. flux, W/m2 */

//#define PARM_CLOUD_NCN	213	/* Amount of non-convective cloud, % */

//#define PARM_CPRAT	214	/* Convective Precipitation rate, kg/m2/s */
//#define PARM_TTDIA	215	/* Temperature tendency by all physics, K/s */

//#define PARM_RDN_TTND	216	/* Temperature tendency by all radiation, Deg.K/s */

//#define PARM_TTPHY	217	/* Temperature tendency by non-radiation physics, K/s */
//#define PARM_PREIX	218	/* precip.index(0.0-1.00)(see note), fraction */
//#define PARM_TSD1D	219	/* Std. dev. of IR T over 1x1 deg area, K */

//#define PARM_LN_PRES	220	/* Natural log of surface pressure, ln(kPa) */
//#define PARM_GPT_HGT5	222	/* 5-wave geopotential height, gpm */

//#define PARM_C_WAT	223	/* Plant canopy surface water, kg/m2 */
//#define PARM_BMIXL	226	/* Blackadar's mixing length scale, m */
//#define PARM_AMIXL	227	/* Asymptotic mixing length scale, m */
//#define PARM_PEVAP	228	/* Potential evaporation, kg/m2 */
//#define PARM_SNOHF	229	/* Snow phase-change heat flux, W/m2 */
//#define PARM_MFLUX	231	/* Convective cloud mas flux, Pa/s */
//#define PARM_DTRF	232	/* Downward total radiation flux, W/m2 */
//#define PARM_UTRF	233	/* Upward total radiation flux, W/m2 */
//#define PARM_BGRUN	234	/* Baseflow-groundwater runoff, kg/m2 */
//#define PARM_SSRUN	235	/* Storm surface runoff, kg/m2 */
//#define PARM_SNO_CVR	238	/* Snow cover, percent */
//#define PARM_SNO_T	239	/* Snow temperature, K */
//#define PARM_LRGHR	241	/* Large scale condensat. heat rate, K/s */
//#define PARM_CNVHR	242	/* Deep convective heating rate, K/s */
//#define PARM_CNVMR	243	/* Deep convective moistening rate, kg/kg/s */
//#define PARM_SHAHR	244	/* Shallow convective heating rate, K/s */
//#define PARM_SHAMR	245	/* Shallow convective moistening rate, kg/kg/s */
//#define PARM_VDFHR	246	/* Vertical diffusion heating rate, K/s */
//#define PARM_VDFUA	247	/* Vertical diffusion zonal acceleration, m/s2 */
//#define PARM_VDFVA	248	/* Vertical diffusion meridional accel, m/s2 */
//#define PARM_VDFMR	249	/* Vertical diffusion moistening rate, kg/kg/s */
//#define PARM_SWHR	250	/* Solar radiative heating rate, K/s */
//#define PARM_LWHR	251	/* long wave radiative heating rate, K/s */
//#define PARM_CD		252	/* Drag coefficient, non-dim */
//#define PARM_FRICV	253	/* Friction velocity, m/s */
//#define PARM_RI		254	/* Richardson number, non-dim. */

//#define PARM_MISSING	255	/* Missing */

/* GRIB 0 parameters that were deleted for GRIB edition 1 */
//#define PARM_VERT_SHR	256	/* vertical wind shear in m/sec/km */
//#define PARM_CON_PRECIP	257	/* convective precip. amount in mm */
//#define PARM_PRECIP     258	/* precipitation amount in mm */
//#define PARM_NCON_PRECIP 259	/* non-convective precip. amount in mm */
//#define PARM_SST_WARM	260	/* afternoon SST warming in degrees C */
//#define PARM_UND_ANOM	261	/* underwater temp. anomaly in degrees C */
//#define PARM_SEA_TEMP_0	262	/* sea temperature in 0.1 degrees C */
//#define PARM_PRESSURE_D 263	/* pressure in 10 pascals */
//#define PARM_GPT_THICK	264	/* geopotential thickness in gpm */
//#define PARM_GPT_HGT_D	265	/* geopotential height in gpm */
//#define PARM_GEOM_HGT_D	266	/* geometrical height in m */
//#define PARM_TEMP_D	267	/* temperature in 0.1 degrees C */
//#define PARM_REL_HUM_D	268	/* relative humididty in 0.1 percent */
//#define PARM_LIFT_INDX_D 269	/* stability (lifted) index in 0.1 degrees C */
//#define PARM_REL_VOR_D	270	/* relative vorticity in 10**-6/sec */
//#define PARM_ABS_VOR_D	271	/* absolute vorticity in 10**-6/sec */
//#define PARM_VERT_VEL_D	272	/* vertical velocity in 10 pascals/sec */
//#define PARM_SEA_TEMP_D	273	/* sea surface temperature in 0.01 degrees C */
//#define PARM_SST_ANOM	274	/* SST anomaly in 0.1 degrees C */
//#define PARM_QUAL_IND	275	/* quality indicators (for generating model) */
//#define PARM_GPT_DEP	276	/* departure from climatological normal geopotential in gpm */
//#define PARM_PRESSURE_DEP 277	/* departure from climatological normal pressure in 100 pascals */

//#define PARM_LAST_ENTRY 278	/* Keep this current if table is expanded.
//				   Must be one more than last defined
//				   parameter code. */


//#define LEVEL_SURFACE	1	/* surface of the Earth */
//#define LEVEL_CLOUD_BASE	2 /* cloud base level */
//#define LEVEL_CLOUD_TOP	3	/* cloud top level */
//#define LEVEL_ISOTHERM	4	/* 0 degree isotherm level */
//#define LEVEL_ADIABAT	5	/* adiabatic condensation level */
//#define LEVEL_MAX_WIND	6	/* maximium wind speed level */
//#define LEVEL_TROP	7	/* at the tropopause */
//#define LEVEL_TOP	8	/* nominal top of atmosphere */
//#define LEVEL_SEABOT	9	/* sea bottom */
//#define LEVEL_ISOBARIC	100	/* isobaric level */
//#define LEVEL_LISO	101	/* layer between two isobaric levels */
//#define LEVEL_MEAN_SEA	102	/* mean sea level */
//#define LEVEL_FH	103	/* fixed height level */
//#define LEVEL_LFHM	104	/* layer between 2 height levels above MSL */
//#define LEVEL_FHG	105	/* fixed height above ground */
//#define LEVEL_LFHG	106	/* layer between 2 height levels above ground */
//#define LEVEL_SIGMA	107	/* sigma level */
//#define LEVEL_LS	108	/* layer between 2 sigma levels */
//#define LEVEL_HY	109	/* Hybrid level */
//#define LEVEL_LHY	110	/* Layer between 2 hybrid levels */
//#define LEVEL_Bls	111	/* Depth below land surface */
//#define LEVEL_LBls	112	/* Layer between 2 depths below land surface */
//#define LEVEL_ISEN	113	/* Isentropic (theta) level */
//#define LEVEL_LISEN	114	/* Layer between 2 isentropic (theta) levels */
//#define LEVEL_PDG	115	/* level at specified pressure difference from ground */
//#define LEVEL_LPDG	116	/* layer between levels at specif. pressure diffs from ground */
//#define LEVEL_PV	117	/* potential vorticity */
//#define LEVEL_LISH	121	/* layer between 2 isobaric surfaces (high precision) */
//#define LEVEL_FHGH	125	/* height level above ground (high precision) */
//#define LEVEL_LSH	128	/* layer between 2 sigma levels (high precision) */
//#define LEVEL_LISM	141	/* layer between 2 isobaric surfaces (mixed precision) */
//#define LEVEL_DBS	160	/* depth below sea level */
//#define LEVEL_ATM	200	/* entire atmosphere considered as a single layer */
//#define LEVEL_OCEAN	201	/* entire ocean considered as a single layer */
