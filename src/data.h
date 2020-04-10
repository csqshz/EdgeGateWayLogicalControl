#ifndef __DATA_H__
#define __DATA_H__

#define START	(1)
#define STOP	(0)
#define COMPLETED	(1)
#define UNCOMPLETED	(0)

#define NORMAL	(false)
#define ALARM	(true)

#define INVALID_DEVICEKEY	(0xffffffff)

#define CHANGED	(1)

enum Season{
	WINTER = 1,
	SUMMER,
	TRANSITION
};

enum CmdOper{
	CMD_WRITE,
	CMD_READ,
};

enum TypeOfVal{
	TypeOfVal_BOOL,
	TypeOfVal_INT,
	TypeOfVal_CHAR,
	TypeOfVal_STRING,
	TypeOfVal_DOUBLE
};

enum ErrorCode{
	ErrCodeSucc = 0,
	ErrCodeFalse
};

typedef union _DataType_u{
	bool		valB;
	int 		valI;
	char		valC;
	double		valD;
	char		valStr[20];
}DataType_u;

typedef struct _PointProp_t{
	char			name[20];

	// 决定Val的类型：enum TypeOfVal, 暂时没用到
	enum TypeOfVal	tag;
	DataType_u		Val;

	unsigned int 	deviceKey;
	char			func[30];

}PointProp_t;

#endif //__DATA_H__