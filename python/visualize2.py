"""
Python visualization for a CSV file
plotting method: plotly
"""

import pandas as pd
import numpy as np

# ============================================================================
# METHOD 3: Interactive Plot with Plotly
# ============================================================================

def calculateDerivedData(df):
    # Find peaks in the 'adc' column and calculate the cycle time of the peaks

    # Create a new columns
    df['time_s'] = df['time_ms']/1000
    # Initialize with NaN
    df['peakIndication'] = float('nan')
    df['peaks'] = float('nan')
    df['dt_ms'] = float('nan')
    peakInhibitCounter = 0
    peaks = []
    iLastPeak = 0
    print(len(df['adc']))
    for i in range(len(df['adc'])-20):
        if (i>20):
            # Step 1: Take three samples, with 20ms+20ms distance (at 2ms sampling cycle time).
            leftValue = df['adc'][i-10]
            midValue = df['adc'][i]
            rightValue = df['adc'][i+10]
            # Step 2: calculate, how much it looks like low-high-low
            peakIndication = (midValue - leftValue + midValue - rightValue)
            if (midValue-leftValue)<220:
                # it is not peak, if the leading edge is too small
                peakIndication = 0
            if (midValue-rightValue)<220:
                # it is not peak, if the trailing edge is too small
                peakIndication = 0
            df.at[i, 'peakIndication']=peakIndication
            if (peakIndication>300) and (peakInhibitCounter==0):
                df.at[i, 'peaks'] = peakIndication
                peakInhibitCounter = 50 # 50 samples is 25ms until we allow to see the next peak
                if (iLastPeak>0): # if we had a peak before, let's calculate the time from the last to the current peak
                    dt_ms = df['time_ms'][i] - df['time_ms'][iLastPeak]
                    df.at[i, 'dt_ms'] = dt_ms
                    if (dt_ms>=60000/250) and (dt_ms<=2000):
                        df.at[i, 'bpm'] = 60000 / dt_ms
                iLastPeak = i
            if (peakInhibitCounter>0):
                peakInhibitCounter-=1



def method3_interactive_plotly(filename):
    """Interactive plot using plotly (requires: pip install plotly)"""
    try:
        import plotly.graph_objects as go
        from plotly.subplots import make_subplots
        
        df = pd.read_csv(filename)
        # we could calculate additional data based on the given data
        calculateDerivedData(df)
        #df['voltage'] = df['adc'] * 5.0 / 1023.0
        
        # Create subplots
        fig = make_subplots(
            rows=3, cols=1,
            row_heights=[0.8, 1.0, 0.15],
            subplot_titles=('Voltage Waveform', 'Heartrate', 'Number of button actuations and Status'),
            shared_xaxes = True, # This syncs the x-axis of all plots.
            vertical_spacing=0.1
        )
        
        # Add ADC trace
        fig.add_trace(
            go.Scatter(x=df['time_s'], y=df['adc'], 
                      mode='lines', name='ADC Value',
                      line=dict(color='blue', width=1)),
            row=1, col=1
        )
        
        # Add bpm
        fig.add_trace(
            go.Scatter(x=df['time_s'], y=df['bpm'],
                      name='Heartrate',
                      mode='markers', 
                      marker=dict( size=5, color='black', opacity=1.0)),
            row=2, col=1
        )

        # Add buttons trace
        fig.add_trace(
            go.Scatter(x=df['time_s'], y=df['buttons'], 
                      name='buttons',
                      mode='markers',
                      marker=dict( size=8, color='orange', opacity=0.7)),
            row=3, col=1
        )

        # Add status trace
        fig.add_trace(
            go.Scatter(x=df['time_s'], y=df['status'], 
                      name='status',
                      mode='markers',
                      marker=dict( size=4, color='green', opacity=0.7)),
            row=3, col=1
        )

        
        # Update layout
        fig.update_xaxes(title_text="Time (s)", row=3, col=1)
        fig.update_yaxes(title_text="ADC Value", row=1, col=1, fixedrange=True) # no y zoom
        fig.update_yaxes(title_text="bpm", row=2, col=1, fixedrange=True) # no y zoom
        fig.update_yaxes(title_text="", row=3, col=1, fixedrange=True) # no y zoom
        
        fig.update_layout(
            title_text="Interactive Data Visualization of " + filename,
            height=800,
            showlegend=True,
            hovermode='x unified'
        )
        
        fig.show(config={'scrollZoom': True})
        
    except ImportError:
        print("Plotly not installed. Run: pip install plotly")



# ============================================================================
# MAIN - Run examples
# ============================================================================

if __name__ == "__main__":
    #filename = '../recordings/DAT00018_2025-12-15_normal_idle.TXT'
    #filename = '../recordings/DAT00048_2025-12-15_jogging_fastmode_slowmode.TXT'
    filename = '../recordings/DAT00071_2025-12-15_jogging_fastmode_slowmode.TXT'
    
    print("Visualization of " + filename + " using plotly")
    method3_interactive_plotly(filename)
