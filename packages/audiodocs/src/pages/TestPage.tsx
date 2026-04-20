import React, { ReactNode } from "react";

import { Container, Spacer } from "@site/src/ui/Layout";
import SliderInput from "@site/src/ui/SliderInput";
import Layout from '@theme/Layout';

import FrequencyResponseGraph from "../examples/Biquad/FrequencyResponseGraph";

const TestPage: React.FC = () => {
  if (process.env.NODE_ENV !== "development") {
    return null;
  }

  return (
    <Layout>
      <Container>
        <div style={{ paddingTop: '120px' }}>
          <h1>Test Page</h1>
          <p>This is a test page for the AudioDocs app.</p>
        </div>

        <Spacer.Vertical size={32} />

        <Section title="Simple UI Components">
          <ControlledSliderExample />
        </Section>

        <Spacer.Vertical size={128} />

        <Section title="Complex UI Components">
          <FrequencyResponseGraph />
        </Section>

        <Spacer.Vertical size={128} />

      </Container>
    </Layout>
  );
};

export default TestPage;

// Helper components and methods

const ControlledSliderExample: React.FC = () => {
  const [value, setValue] = React.useState(50);

  return (
    <SliderInput
      label="Slider"
      value={value}
      min={0}
      max={100}
      step={1}
      onChange={(nextValue) => setValue(nextValue)}
    />
  );
};

interface SectionProps {
  title: string;
  children?: ReactNode | ReactNode[];
}

const Section: React.FC<SectionProps> = ({ title, children }) => (
  <div>
    <h2 style={{ marginBottom: 16 }}>{title}</h2>
    {children}
  </div>
);
