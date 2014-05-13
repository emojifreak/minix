mode_t get_mode(struct inode *ino, int mode)
int do_stat(void)
  ino_t ino_nr;
  ino_nr = m_in.m_vfs_fs_stat.inode;
  stat.st_size = attr.a_size;
  return sys_safecopyto(m_in.m_source, m_in.m_vfs_fs_stat.grant, 0,
int do_chmod(void)
  if ((ino = find_inode(m_in.m_vfs_fs_chmod.inode)) == NULL)
  attr.a_mode = m_in.m_vfs_fs_chmod.mode; /* no need to convert in this direction */
  m_out.m_fs_vfs_chmod.mode = get_mode(ino, attr.a_mode);
int do_utime(void)
  if ((ino = find_inode(m_in.m_vfs_fs_utime.inode)) == NULL)
  switch(m_in.m_vfs_fs_utime.acnsec) {
	m_in.m_vfs_fs_utime.acnsec = 0;
	/* cases m_in.m_vfs_fs_utime.acnsec < 0 || m_in.m_vfs_fs_utime.acnsec >= 1E9
	attr.a_atime.tv_sec = m_in.m_vfs_fs_utime.actime;
	attr.a_atime.tv_nsec = m_in.m_vfs_fs_utime.acnsec;
  switch(m_in.m_vfs_fs_utime.modnsec) {
	m_in.m_vfs_fs_utime.modnsec = 0;
	/* cases m_in.m_vfs_fs_utime.modnsec < 0 || m_in.m_vfs_fs_utime.modnsec >= 1E9
	attr.a_mtime.tv_sec = m_in.m_vfs_fs_utime.modtime;
	attr.a_mtime.tv_nsec = m_in.m_vfs_fs_utime.modnsec;