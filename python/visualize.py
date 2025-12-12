"""
Python visualization for a CSV file
Multiple plotting methods: matplotlib, pandas, and plotly
"""

import pandas as pd
import matplotlib.pyplot as plt
import numpy as np

# ============================================================================
# METHOD 1: Using Pandas and Matplotlib (Most Common)
# ============================================================================

def method1_basic_plot(filename):
    """Simple plot using pandas and matplotlib"""
    # Read the CSV file
    df = pd.read_csv(filename)
    
    # Basic plot
    plt.figure(figsize=(12, 6))
    plt.plot(df['time_ms'], df['adc'], linewidth=0.5, color='blue')
    plt.xlabel('Time (ms)', fontsize=12)
    plt.ylabel('ADC Value', fontsize=12)
    plt.title('ADC Waveform - Full Dataset', fontsize=14, fontweight='bold')
    plt.grid(True, alpha=0.3)
    plt.tight_layout()
    plt.show()
    
    # Print statistics
    print("=== Data Statistics ===")
    print(f"Total samples: {len(df)}")
    print(f"Duration: {df['time_ms'].max() / 1000:.1f} seconds")
    print(f"Min ADC: {df['adc'].min()}")
    print(f"Max ADC: {df['adc'].max()}")
    print(f"Mean ADC: {df['adc'].mean():.1f}")
    print(f"Std Dev: {df['adc'].std():.1f}")


# ============================================================================
# METHOD 2: Multiple Subplots with Analysis
# ============================================================================

def method2_detailed_analysis(filename):
    """Detailed analysis with multiple subplots"""
    df = pd.read_csv(filename)
    
    # Convert ADC to voltage (assuming 5V reference)
    df['voltage'] = df['adc'] * 5.0 / 1023.0
    
    fig, axes = plt.subplots(3, 1, figsize=(14, 10))
    
    # Plot 1: Full waveform
    axes[0].plot(df['time_ms'], df['adc'], linewidth=0.5, color='blue')
    axes[0].set_xlabel('Time (ms)')
    axes[0].set_ylabel('ADC Value')
    axes[0].set_title('Complete Waveform', fontweight='bold')
    axes[0].grid(True, alpha=0.3)
    
    # Plot 2: Zoomed view (first 1 second)
    df_zoom = df[df['time_ms'] <= 1000]
    axes[1].plot(df_zoom['time_ms'], df_zoom['adc'], linewidth=1, color='green')
    axes[1].set_xlabel('Time (ms)')
    axes[1].set_ylabel('ADC Value')
    axes[1].set_title('Zoomed View - First 1 Second', fontweight='bold')
    axes[1].grid(True, alpha=0.3)
    
    # Plot 3: Histogram
    axes[2].hist(df['adc'], bins=50, color='orange', alpha=0.7, edgecolor='black')
    axes[2].set_xlabel('ADC Value')
    axes[2].set_ylabel('Frequency')
    axes[2].set_title('ADC Value Distribution', fontweight='bold')
    axes[2].grid(True, alpha=0.3)
    
    plt.tight_layout()
    plt.show()


# ============================================================================
# METHOD 3: Interactive Plot with Plotly
# ============================================================================

def method3_interactive_plotly(filename):
    """Interactive plot using plotly (requires: pip install plotly)"""
    try:
        import plotly.graph_objects as go
        from plotly.subplots import make_subplots
        
        df = pd.read_csv(filename)
        # we could calculate additional data based on the given data
        df['voltage'] = df['adc'] * 5.0 / 1023.0
        
        # Create subplots
        fig = make_subplots(
            rows=3, cols=1,
            subplot_titles=('ADC Waveform', 'Voltage (V)', 'Number of button actuations and Status'),
            shared_xaxes = True, # This syncs the x-axis of all plots.
            vertical_spacing=0.1
        )
        
        # Add ADC trace
        fig.add_trace(
            go.Scatter(x=df['time_ms'], y=df['adc'], 
                      mode='lines', name='ADC Value',
                      line=dict(color='blue', width=1)),
            row=1, col=1
        )
        
        # Add voltage trace
        fig.add_trace(
            go.Scatter(x=df['time_ms'], y=df['voltage'], 
                      mode='lines', name='Voltage',
                      line=dict(color='red', width=1)),
            row=2, col=1
        )

        # Add buttons trace
        fig.add_trace(
            go.Scatter(x=df['time_ms'], y=df['buttons'], 
                      name='buttons',
                      mode='markers',
                      marker=dict( size=30, color='orange', opacity=0.7)),
            row=3, col=1
        )

        # Add status trace
        fig.add_trace(
            go.Scatter(x=df['time_ms'], y=df['status'], 
                      name='status',
                      mode='markers',
                      marker=dict( size=10, color='green', opacity=0.7)),
            row=3, col=1
        )

        
        # Update layout
        fig.update_xaxes(title_text="Time (ms)", row=2, col=1)
        fig.update_yaxes(title_text="ADC Value", row=1, col=1, fixedrange=True) # no y zoom
        fig.update_yaxes(title_text="Voltage (V)", row=2, col=1)
        fig.update_yaxes(title_text="number of button actuations", row=3, col=1, fixedrange=True) # no y zoom
        # X-axis: zoomable, Y-axis: fixed
        #fig.update_xaxes(fixedrange=False)
        #fig.update_yaxes(fixedrange=True)
        
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
# METHOD 5: Save plots to file
# ============================================================================

def method5_save_plots(filename, output='waveform.png'):
    """Generate and save plot to file"""
    df = pd.read_csv(filename)
    
    plt.figure(figsize=(14, 6))
    plt.plot(df['time_ms'], df['adc'], linewidth=0.5, color='blue', alpha=0.8)
    plt.xlabel('Time (ms)', fontsize=12)
    plt.ylabel('ADC Value', fontsize=12)
    plt.title('ADC Waveform Analysis', fontsize=14, fontweight='bold')
    plt.grid(True, alpha=0.3)
    
    # Add statistics text
    stats_text = f"Min: {df['adc'].min()} | Max: {df['adc'].max()} | Mean: {df['adc'].mean():.1f}"
    plt.text(0.02, 0.98, stats_text, transform=plt.gca().transAxes,
             fontsize=10, verticalalignment='top',
             bbox=dict(boxstyle='round', facecolor='wheat', alpha=0.5))
    
    plt.tight_layout()
    plt.savefig(output, dpi=300, bbox_inches='tight')
    print(f"Plot saved to {output}")
    plt.show()


# ============================================================================
# MAIN - Run examples
# ============================================================================

if __name__ == "__main__":
    filename = 'A00042_2025-12-11.TXT'
    
    print("Choose visualization method:")
    print("1. Basic plot")
    print("2. Detailed analysis (3 subplots)")
    print("3. Interactive plot (Plotly)")
    print("5. Save plot to file")
    
    choice = input("\nEnter choice (1-6): ").strip()
    
    if choice == '1':
        method1_basic_plot(filename)
    elif choice == '2':
        method2_detailed_analysis(filename)
    elif choice == '3':
        method3_interactive_plotly(filename)
    elif choice == '5':
        method5_save_plots(filename)
    else:
        print("Invalid choice. Running basic plot...")
        method1_basic_plot(filename)