/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package com.xilinx.laf;
import java.awt.Graphics2D;
/**
 *
 * @author testadvs
 */
public interface Painter<T> {
    public void paint(Graphics2D g, T object, int width, int height);
}
