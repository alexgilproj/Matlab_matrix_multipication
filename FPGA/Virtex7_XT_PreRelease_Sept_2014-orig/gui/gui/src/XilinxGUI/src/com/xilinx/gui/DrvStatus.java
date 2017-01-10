/*  1:   */ package com.xilinx.gui;
/*  2:   */ 
/*  3:   */ public class DrvStatus
/*  4:   */ {
/*  5:   */   public native int getBoardNumber();
/*  6:   */   
/*  7:   */   public native int getMode(int paramInt);
/*  8:   */   
/*  9:   */   public native int getRawMode(int paramInt);
/* 10:   */   
/* 11:   */   public native int getTestConfig(int paramInt);
/* 12:   */   
/* 13:   */   public native void setMode(int paramInt1, int paramInt2);
/* 14:   */   
/* 15:   */   public native void setRawMode(int paramInt1, int paramInt2);
/* 16:   */   
/* 17:   */   public native void setTestConfig(int paramInt1, int paramInt2);
/* 18:   */   
/* 19:   */   public native void cmdEnable(int paramInt);
/* 20:   */   
/* 21:   */   public native void cmdDisable(int paramInt);
/* 22:   */   
/* 23:   */   static
/* 24:   */   {
/* 25:13 */     System.loadLibrary("drvstatus");
/* 26:   */   }
/* 27:   */ }
