
interface ExampleRowProps {
  title: string;
  description: string;
  Component?: React.FC;
  inverted?: boolean;
}

const ExampleRow: React.FC<ExampleRowProps> = ({ title, description, Component }) => {
  return (
    <div></div>
  );
};

export default ExampleRow;
