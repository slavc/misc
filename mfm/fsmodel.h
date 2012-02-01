#ifndef FSMODEL_H
#define FSMODEL_H

#include <gdkconfig.h>
#include <gtk/gtktreemodel.h>
#include <gtk/gtktreesortable.h>


G_BEGIN_DECLS


/* cu stands for CUstom widget */

#define CU_TYPE_FSMODEL			(cu_fsmodel_get_type ())
#define CU_FSMODEL(obj)			(G_TYPE_CHECK_INSTANCE_CAST ((obj), CU_TYPE_FSMODEL, CuFsmodel))
#define CU_FSMODEL_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST ((klass), CU_TYPE_FSMODEL, CuFsmodelClass))
#define CU_IS_FSMODEL(obj)			(G_TYPE_CHECK_INSTANCE_TYPE ((obj), CU_TYPE_FSMODEL))
#define CU_IS_FSMODEL(klass)		(G_TYPE_CHECK_CLASS_TYPE ((klass), CU_TYPE_FSMODEL))
#define CU_FSMODEL_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS ((obj), CU_TYPE_FSMODEL, CuFsmodelClass))

typedef struct _CuFsmodel       CuFsmodel;
typedef struct _CuFsmodelClass  CuFsmodelClass;

#endif
