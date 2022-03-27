/*********************************************/
/* by adventuresin9                          */
/*                                           */
/* This provides a file system for the       */
/* BH1750 Ambient Light Sensor.              */
/*                                           */
/* While reading the "lux" file outputs just */
/* one reading, it is set to continuous mode.*/
/* So file reads can be looped to get new    */
/* readings every 120ms.                     */
/*********************************************/



#include <u.h>
#include <libc.h>
#include <fcall.h>
#include <thread.h>
#include <9p.h>


typedef struct Devfile Devfile;

void	rend(Srv *);
void	ropen(Req *r);
void	rread(Req *r);
void	initfs(char *dirname);
int		initchip(void);
void	closechip(void);
char*	readlux(Req *r);


struct Devfile {
	char	*name;
	char*	(*rread)(Req*);
	int	mode;
};


Devfile files[] = {
	{ "lux", readlux, DMEXCL|0444 },
};


Srv s = {
	.open = ropen,
	.read = rread,
	.end = rend,
};


File *root;
File *devdir;
int i²cfd;


void
rend(Srv *)
{
	closechip();
}


void
ropen(Req *r)
{
	respond(r, nil);
}


void
rread(Req *r)
{
	Devfile *f;

	r->ofcall.count = 0;
	f = r->fid->file->aux;
	respond(r, f->rread(r));
}


void
initfs(char *dirname)
{
	char* user;
	int i;

	user = getuser();
	s.tree = alloctree(user, user, 0555, nil);
	if(s.tree == nil)
		sysfatal("initfs: alloctree: %r");
	root = s.tree->root;
	if((devdir = createfile(root, dirname, user, DMDIR|0555, nil)) == nil)
		sysfatal("initfs: createfile: bh1750: %r");
	for(i = 0; i < nelem(files); i++)
		if(createfile(devdir, files[i].name, user, files[i].mode, files + i) == nil)
			sysfatal("initfs: createfile: %s: %r", files[i].name);
}


int
initchip(void)
{
	int fd = -1;
	uchar buf[1];

	if(access("/dev/i2c.23.data", 0) != 0)
		bind("#J23", "/dev", MBEFORE);

	fd = open("/dev/i2c.23.data", ORDWR);
	if(fd < 0)
		sysfatal("cant open file");

	buf[0] = 0x01;  /* power on */

	pwrite(fd, buf, 1, 0);

	sleep(150);

	buf[0] = 0x10; /* Continuously H-Resolution Mode */

	pwrite(fd, buf, 1, 0);


	return(fd);
}


void
closechip(void)
{
	uchar buf[1];

	buf[0] = 0x00;

	pwrite(i²cfd, buf, 1, 0);

	close(i²cfd);
	threadexitsall(nil);
}


char*
readlux(Req *r)
{
	uchar buf[2];
	char out[256], *p;
	int lux;

	memset(buf, 0, 2);
	memset(out, 0, 256);

	pread(i²cfd, buf, 2, 0);

	lux = (buf[0] << 8) | buf[1];
	lux = lux / 1.2;

	p = out;
	p = seprint(p, out + sizeof out, "%d lux\n", lux);
	USED(p);

	readstr(r, out);
	return nil;
}


void
threadmain(int argc, char *argv[])
{
	char *srvname, *mntpt;

	srvname = "bh1750";
	mntpt = "/mnt";

	ARGBEGIN {
	default:
		fprint(2, "usage: %s [-m mntpt] [-s srvname]\n", argv0);
		exits("usage");
	case 's':
		srvname = ARGF();
		break;
	case 'm':
		mntpt = ARGF();
		break;
	} ARGEND


	initfs(srvname);
	i²cfd = initchip();
	threadpostmountsrv(&s, srvname, mntpt, MBEFORE);
	threadexits(nil);
}
