#include <iostream>
#include <sciplot/sciplot.hpp>
using namespace sciplot;

int main(int argc, char** argv)
{
    // Create values for your x-axis
    Vec x = linspace(0.0, 5.0, 100);

    // Create a Plot object
    Plot2D plot1;
    // Set color palette for first Plot
    plot1.palette("paired");
    std::cout << "Palette set\n";
    // Draw a sine graph putting x on the x-axis and sin(x) on the y-axis
    plot1.drawCurve(x, std::sin(x)).label("sin(x)").lineWidth(4);
    // Draw a cosine graph putting x on the x-axis and cos(x) on the y-axis
    plot1.drawCurve(x, std::cos(x)).label("cos(x)").lineWidth(2);
    std::cout << "Sin Cos drawn\n";

    // Create a second Plot object
    Plot2D plot2;
    // Draw a tangent graph putting x on the x-axis and tan(x) on the y-axis
    plot2.drawCurve(x, std::tan(x)).label("tan(x)").lineWidth(4);

    // Put both plots in a "figure" horizontally next to each other
    Figure figure = {{plot1, plot2}};

    // Create a canvas / drawing area to hold figure and plots
    Canvas canvas = {{figure}};
    // Set color palette for all Plots that do not have a palette set (plot2) / the default palette
    canvas.defaultPalette("set1");

    // Show the canvas in a pop-up window
    canvas.show();

    // Save the plot to a SVG file
    canvas.save("example-readme.svg");
}